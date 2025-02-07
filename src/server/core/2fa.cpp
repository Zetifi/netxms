/* 
** NetXMS - Network Management System
** Copyright (C) 2021-2022 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: 2fa.cpp
**
**/

#include "nxcore.h"
#include <nxcore_2fa.h>

#define DEBUG_TAG _T("2fa")

/**
 * TOTP token constructor
 */
TOTPToken::TOTPToken(const TCHAR* methodName, const BYTE* secret, const TCHAR *userName, bool newSecret) : TwoFactorAuthenticationToken(methodName)
{
   memcpy(m_secret, secret, TOTP_SECRET_LENGTH);
   m_newSecret = newSecret;
   if (newSecret)
   {
      char issuer[256] = "NetXMS (";
      ConfigReadStrUTF8(_T("Server.Name"), &issuer[8], 240, "");
      if (issuer[8] == 0)
         GetLocalIpAddr().toStringA(&issuer[8]);
      strcat(issuer, ")");

      char urlEncodedIssuer[1024];
      URLEncode(issuer, urlEncodedIssuer, sizeof(urlEncodedIssuer));

      char encodedSecret[TOTP_SECRET_LENGTH * 2];
      base32_encode(reinterpret_cast<const char*>(secret), TOTP_SECRET_LENGTH, encodedSecret, sizeof(encodedSecret));
      size_t slen = strlen(encodedSecret);
      if (encodedSecret[slen - 1] == '=')
         encodedSecret[slen - 1] = 0;  // Remove Base32 padding

      char uri[4096] = "otpauth://totp/";
      strcat(uri, urlEncodedIssuer);
      strcat(uri, ":");
      size_t l = strlen(uri);
      tchar_to_utf8(userName, -1, &uri[l], sizeof(uri) - l);
      strlcat(uri, "?issuer=", sizeof(uri));
      strlcat(uri, urlEncodedIssuer, sizeof(uri));
      strlcat(uri, "&digits=6&algorithm=SHA1&secret=", sizeof(uri));
      strlcat(uri, encodedSecret, sizeof(uri));

      m_uri = WideStringFromUTF8String(uri);
   }
   else
   {
      m_uri = nullptr;
   }
}

/**
 * TOTP token destructor
 */
TOTPToken::~TOTPToken()
{
   MemFree(m_uri);
};

/**
 * Authentication method base class
 */
class TwoFactorAuthenticationMethod
{
protected:
   TCHAR m_methodName[MAX_OBJECT_NAME];
   TCHAR m_description[MAX_2FA_DESCRIPTION];
   bool m_isValid;

   TwoFactorAuthenticationMethod(const TCHAR* name, const TCHAR* description, const Config& config);

public:
   virtual ~TwoFactorAuthenticationMethod() = default;

   virtual const TCHAR *getDriverName() const = 0;
   virtual TwoFactorAuthenticationToken* prepareChallenge(uint32_t userId) = 0;
   virtual bool validateResponse(TwoFactorAuthenticationToken *token, const TCHAR *response, uint32_t userId) = 0;
   virtual unique_ptr<StringMap> extractBindingConfiguration(const Config& binding) const = 0;
   virtual void updateBindingConfiguration(Config* binding, const StringMap& updates) const = 0;

   bool isValid() const { return m_isValid; };
   const TCHAR* getName() const { return m_methodName; };
   const TCHAR* getDescription() const { return m_description; };

   bool saveToDatabase(const char *configuration) const;
};

static StringObjectMap<TwoFactorAuthenticationMethod> s_methods(Ownership::True);
static Mutex s_authMethodListLock;

/**
 * Authentication method base constructor
 */
TwoFactorAuthenticationMethod::TwoFactorAuthenticationMethod(const TCHAR* name, const TCHAR* description, const Config& config)
{
   m_isValid = false;
   _tcslcpy(m_methodName, name, MAX_OBJECT_NAME);
   _tcslcpy(m_description, description, MAX_OBJECT_NAME);
}

/**
 * Save 2FA method to database
 */
bool TwoFactorAuthenticationMethod::saveToDatabase(const char *configuration) const
{
   bool success = false;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("two_factor_auth_methods"), _T("name"), m_methodName))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE two_factor_auth_methods SET driver=?,description=?,configuration=? WHERE name=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO two_factor_auth_methods (driver,description,configuration,name) VALUES (?,?,?,?)"));
   }
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, getDriverName(), DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, configuration, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_methodName, DB_BIND_STATIC);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return success;
}

/**
 * TOTP authentication method
 */
class TOTPAuthMethod : public TwoFactorAuthenticationMethod
{
public:
   TOTPAuthMethod(const TCHAR* name, const TCHAR* description, const Config& config) : TwoFactorAuthenticationMethod(name, description, config)
   {
      m_isValid = true;
   }

   virtual const TCHAR *getDriverName() const override { return _T("TOTP"); }
   virtual TwoFactorAuthenticationToken* prepareChallenge(uint32_t userId) override;
   virtual bool validateResponse(TwoFactorAuthenticationToken* token, const TCHAR* response, uint32_t userId) override;
   virtual unique_ptr<StringMap> extractBindingConfiguration(const Config& binding) const override;
   virtual void updateBindingConfiguration(Config* binding, const StringMap& updates) const override;
};

/**
 * Preparing challenge for user with TOTP
 */
TwoFactorAuthenticationToken* TOTPAuthMethod::prepareChallenge(uint32_t userId)
{
   TwoFactorAuthenticationToken* token = nullptr;
   shared_ptr<Config> binding = GetUser2FAMethodBinding(userId, m_methodName);
   if (binding != nullptr)
   {
      BYTE secretBytes[TOTP_SECRET_LENGTH];
      bool newSecret;
      const TCHAR* secret = binding->getValue(_T("/MethodBinding/Secret"));
      if (binding->getValueAsBoolean(_T("/MethodBinding/Initialized"), false) && (secret != nullptr))
      {
         StrToBin(secret, secretBytes, TOTP_SECRET_LENGTH);
         newSecret = false;
      }
      else
      {
         RAND_bytes(secretBytes, TOTP_SECRET_LENGTH);
         newSecret = true;
      }
      TCHAR userName[MAX_USER_NAME];
      token = new TOTPToken(m_methodName, secretBytes, ResolveUserId(userId, userName, true), newSecret);
   }
   return token;
}

/**
 * Validate response from user
 */
bool TOTPAuthMethod::validateResponse(TwoFactorAuthenticationToken* token, const TCHAR* response, uint32_t userId)
{
   if (_tcscmp(token->getMethodName(), m_methodName))
      return false;

   auto totpToken = static_cast<TOTPToken*>(token);
   time_t now = time(nullptr);
   for (uint64_t i = 0; i < 3; i++) // we will test current time factor, previous and next
   {
      uint64_t timeFactor = htonq(((now / _ULL(30)) + i) - _ULL(2));
      BYTE hash[SHA1_DIGEST_SIZE];
      uint32_t hashLen;
      HMAC(EVP_sha1(), totpToken->getSecret(), TOTP_SECRET_LENGTH, reinterpret_cast<const BYTE*>(&timeFactor), sizeof(uint64_t), hash, &hashLen);
      int offset = static_cast<int>(hash[SHA1_DIGEST_SIZE - 1] & 0x0F);
      uint32_t challenge;
      memcpy(&challenge, &hash[offset], sizeof(uint32_t));
      challenge = (ntohl(challenge) & 0x7FFFFFFF) % 1000000;
      if (challenge == _tcstol(response, nullptr, 10))
      {
         if (totpToken->isNewSecret())
         {
            shared_ptr<Config> binding = GetUser2FAMethodBinding(userId, m_methodName);
            if (binding != nullptr)
            {
               TCHAR secretText[256];
               BinToStr(totpToken->getSecret(), TOTP_SECRET_LENGTH, secretText);
               binding->setValue(_T("/MethodBinding/Secret"), secretText);
               binding->setValue(_T("/MethodBinding/Initialized"), 1);
               MarkUserDatabaseObjectAsModified(userId);
            }
         }
         return true;
      }
   }
   return false;
}

/**
 * Extract binding configuration
 */
unique_ptr<StringMap> TOTPAuthMethod::extractBindingConfiguration(const Config& binding) const
{
   auto configuration = make_unique<StringMap>();
   configuration->set(_T("Initialized"), binding.getValueAsBoolean(_T("/MethodBinding/Initialized"), false) ? _T("true") : _T("false"));
   return configuration;
}

/**
 * Update binding configuration
 */
void TOTPAuthMethod::updateBindingConfiguration(Config* binding, const StringMap& updates) const
{
   binding->setValue(_T("/MethodBinding/Initialized"), updates.getBoolean(_T("Initialized"), false) ? 1 : 0);
}

/**
 * Notification channel authentication class
 */
class MessageAuthMethod : public TwoFactorAuthenticationMethod
{
private:
   TCHAR m_channelName[MAX_OBJECT_NAME];

public:
   MessageAuthMethod(const TCHAR* name, const TCHAR* description, const Config& config);

   virtual const TCHAR *getDriverName() const override { return _T("Message"); }
   virtual TwoFactorAuthenticationToken* prepareChallenge(uint32_t userId) override;
   virtual bool validateResponse(TwoFactorAuthenticationToken* token, const TCHAR* response, uint32_t userId) override;
   virtual unique_ptr<StringMap> extractBindingConfiguration(const Config& binding) const override;
   virtual void updateBindingConfiguration(Config* binding, const StringMap& updates) const override;
};

/**
 * Notification channel authentication class constructor
 */
MessageAuthMethod::MessageAuthMethod(const TCHAR* name, const TCHAR* description, const Config& config) : TwoFactorAuthenticationMethod(name, description, config)
{
   const TCHAR* channelName = config.getValue(_T("/MethodConfiguration/ChannelName"));
   if (channelName != nullptr)
   {
      _tcslcpy(m_channelName, channelName, MAX_OBJECT_NAME);
      m_isValid = true;
   }
   else
   {
      m_channelName[0] = 0;
   }
}

/**
 * Preparing challenge for user using notification channel
 */
TwoFactorAuthenticationToken* MessageAuthMethod::prepareChallenge(uint32_t userId)
{
   shared_ptr<Config> binding = GetUser2FAMethodBinding(userId, m_methodName);
   if (binding == nullptr)
      return nullptr;

   uint32_t challenge;
   GenerateRandomBytes(reinterpret_cast<BYTE*>(&challenge), sizeof(challenge));
   challenge = challenge % 1000000; // We will use 6 digits code
   TCHAR challengeStr[7];
   _sntprintf(challengeStr, 7, _T("%06u"), challenge);

   TCHAR userName[MAX_USER_NAME];
   TCHAR *recipient = MemCopyString(binding->getValue(_T("/MethodBinding/Recipient"), ResolveUserId(userId, userName, true)));
   const TCHAR *subject = binding->getValue(_T("/MethodBinding/Subject"), _T("NetXMS two-factor authentication code"));
   SendNotification(m_channelName, recipient, subject, challengeStr);
   MemFree(recipient);

   return new MessageToken(m_methodName, challenge);
}

/**
 * Validate response from user
 */
bool MessageAuthMethod::validateResponse(TwoFactorAuthenticationToken *token, const TCHAR *response, uint32_t userId)
{
   if (_tcscmp(token->getMethodName(), m_methodName))
      return false;

   return static_cast<MessageToken*>(token)->getSecret() == _tcstol(response, nullptr, 10);
}

/**
 * Extract binding configuration
 */
unique_ptr<StringMap> MessageAuthMethod::extractBindingConfiguration(const Config& binding) const
{
   auto configuration = make_unique<StringMap>();
   configuration->set(_T("Recipient"), binding.getValue(_T("/MethodBinding/Recipient"), _T("")));
   configuration->set(_T("Subject"), binding.getValue(_T("/MethodBinding/Subject"), _T("")));
   return configuration;
}

/**
 * Update binding configuration
 */
void MessageAuthMethod::updateBindingConfiguration(Config* binding, const StringMap& updates) const
{
   binding->setValue(_T("/MethodBinding/Recipient"), CHECK_NULL_EX(updates.get(_T("Recipient"))));
   binding->setValue(_T("/MethodBinding/Subject"), CHECK_NULL_EX(updates.get(_T("Subject"))));
}

/**
 * Create new two-factor authentication driver instance based on configuration data
 */
static TwoFactorAuthenticationMethod* CreateAuthenticationMethod(const TCHAR* name, const TCHAR* driver, const TCHAR* description, const char* configData)
{
   TwoFactorAuthenticationMethod* method = nullptr;
   Config config;
   if (config.loadConfigFromMemory(configData, strlen(configData), _T("MethodConfiguration"), nullptr, true, false))
   {
      if (!_tcscmp(driver, _T("TOTP")))
      {
         method = new TOTPAuthMethod(name, description, config);
      }
      else if (!_tcscmp(driver, _T("Message")))
      {
         method = new MessageAuthMethod(name, description, config);
      }

      if (method == nullptr)
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot find driver \"%s\" for two-factor authentication method \"%s\""), driver, name);
      }
      else if (!method->isValid())
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Two-factor authentication method \"%s\" (driver = \"%s\") initialization failed"), name, driver);
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Configuration parsing failed for two-factor authentication method \"%s\" (configuration: %hs)"), name, configData);
   }
   return method;
}

/**
 * 2FA method information
 */
class TwoFactorAuthMethodInfo
{
private:
   TCHAR m_name[MAX_OBJECT_NAME];
   TCHAR m_driver[MAX_OBJECT_NAME];
   TCHAR m_description[MAX_2FA_DESCRIPTION];
   char *m_configuration;

public:
   TwoFactorAuthMethodInfo(DB_RESULT hResult, int row)
   {
      DBGetField(hResult, row, 0, m_name, MAX_OBJECT_NAME);
      DBGetField(hResult, row, 1, m_driver, MAX_OBJECT_NAME);
      DBGetField(hResult, row, 2, m_description, MAX_2FA_DESCRIPTION);
      m_configuration = DBGetFieldUTF8(hResult, row, 3, nullptr, 0);
   }

   ~TwoFactorAuthMethodInfo()
   {
      MemFree(m_configuration);
   }

   const TCHAR *getName() const { return m_name; };
   const TCHAR *getDriver() const { return m_driver; };
   const TCHAR *getDescription() const { return m_description; };
   const char *getConfiguration() const { return m_configuration; };
};

/**
 * Reading 2FA methods info from DB
 */
unique_ptr<ObjectArray<TwoFactorAuthMethodInfo>> Load2FAMethodsInfoFromDB()
{
   auto methods = make_unique<ObjectArray<TwoFactorAuthMethodInfo>>(0, 32, Ownership::True);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT name,driver,description,configuration FROM two_factor_auth_methods"));
   if (hResult != nullptr)
   {
      int numRows = DBGetNumRows(hResult);
      for (int i = 0; i < numRows; i++)
      {
         methods->add(new TwoFactorAuthMethodInfo(hResult, i));
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return methods;
}

/**
 * Load two-factor authentication methods from database
 */
void LoadTwoFactorAuthenticationMethods()
{
   int numberOfAddedMethods = 0;
   unique_ptr<ObjectArray<TwoFactorAuthMethodInfo>> methodsInfo = Load2FAMethodsInfoFromDB();
   for (int i = 0; i < methodsInfo->size(); i++)
   {
      TwoFactorAuthMethodInfo *info = methodsInfo->get(i);
      if (info->getConfiguration() != nullptr)
      {
         TwoFactorAuthenticationMethod *am = CreateAuthenticationMethod(info->getName(), info->getDriver(), info->getDescription(), info->getConfiguration());
         if (am != nullptr)
         {
            s_authMethodListLock.lock();
            s_methods.set(info->getName(), am);
            s_authMethodListLock.unlock();
            numberOfAddedMethods++;
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Two-factor authentication method \"%s\" successfully created"), info->getName());
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Two-factor authentication method \"%s\" creation failed"), info->getName());
         }
      }
      else
      {
         nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Unable to read configuration for two-factor authentication method \"%s\""), info->getName());
      }
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d two-factor authentication methods added"), numberOfAddedMethods);
}

/**
 * Prepare 2FA challenge for user using selected method
 */
TwoFactorAuthenticationToken* Prepare2FAChallenge(const TCHAR* methodName, uint32_t userId)
{
   TwoFactorAuthenticationToken* token = nullptr;
   s_authMethodListLock.lock();
   TwoFactorAuthenticationMethod *am = s_methods.get(methodName);
   if (am != nullptr)
   {
      token = am->prepareChallenge(userId);
   }
   s_authMethodListLock.unlock();
   return token;
}

/**
 * Validate 2FA response
 */
bool Validate2FAResponse(TwoFactorAuthenticationToken* token, TCHAR *response, uint32_t userId)
{
   bool success = false;
   s_authMethodListLock.lock();
   if (token != nullptr)
   {
      TwoFactorAuthenticationMethod *am = s_methods.get(token->getMethodName());
      if (am != nullptr)
      {
         success = am->validateResponse(token, response, userId);
      }
   }
   s_authMethodListLock.unlock();
   return success;
}

/**
 * Adds 2FA method details info from DB to message
 */
void Get2FAMethodDetails(const TCHAR* name, NXCPMessage *msg)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelectFormatted(hdb, _T("SELECT driver,description,configuration FROM two_factor_auth_methods WHERE name=%s"), DBPrepareString(hdb, name).cstr());
   if (result != nullptr)
   {
      if (DBGetNumRows(result) > 0)
      {
         TCHAR driver[MAX_OBJECT_NAME], description[MAX_2FA_DESCRIPTION];
         DBGetField(result, 0, 0, driver, MAX_OBJECT_NAME);
         DBGetField(result, 0, 1, description, MAX_2FA_DESCRIPTION);
         TCHAR *configuration = DBGetField(result, 0, 2, nullptr, 0);

         msg->setField(VID_NAME, name);
         msg->setField(VID_DRIVER_NAME, driver);
         msg->setField(VID_DESCRIPTION, description);
         msg->setField(VID_CONFIG_FILE_DATA, configuration);

         s_authMethodListLock.lock();
         msg->setField(VID_IS_ACTIVE, s_methods.contains(name));
         s_authMethodListLock.unlock();

         MemFree(configuration);
      }
      DBFreeResult(result);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Fills message with 2FA methods list
 */
void Get2FAMethods(NXCPMessage *msg)
{
   unique_ptr<ObjectArray<TwoFactorAuthMethodInfo>> methodsInfo = Load2FAMethodsInfoFromDB();
   uint32_t fieldId = VID_2FA_METHOD_LIST_BASE;
   s_authMethodListLock.lock();
   for (int i = 0; i < methodsInfo->size(); i++)
   {
      TwoFactorAuthMethodInfo *method = methodsInfo->get(i);
      msg->setField(fieldId++, method->getName());
      msg->setField(fieldId++, method->getDescription());
      msg->setField(fieldId++, method->getDriver());
      msg->setFieldFromUtf8String(fieldId++, method->getConfiguration());
      msg->setField(fieldId++, s_methods.contains(method->getName()));
      fieldId += 5;
   }
   s_authMethodListLock.unlock();
   msg->setField(VID_2FA_METHOD_COUNT, methodsInfo->size());
}

/**
 * Updates or creates new authentication method
 */
uint32_t Modify2FAMethod(const TCHAR* name, const TCHAR* methodType, const TCHAR* description, const char *configuration)
{
   uint32_t rcc = RCC_NO_SUCH_2FA_DRIVER;
   TwoFactorAuthenticationMethod* am = CreateAuthenticationMethod(name, methodType, description, configuration);
   if (am != nullptr)
   {
      if (am->saveToDatabase(configuration))
      {
         if (am->isValid())
         {
            s_authMethodListLock.lock();
            s_methods.set(name, am);
            s_authMethodListLock.unlock();
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Two-factor authentication method \"%s\" successfully registered"), name);
         }
         else
         {
            s_authMethodListLock.lock();
            s_methods.remove(name);
            s_authMethodListLock.unlock();
            delete am;
         }
         rcc = RCC_SUCCESS;
         NotifyClientSessions(NX_NOTIFY_2FA_METHOD_CHANGED, 0);
      }
      else
      {
         rcc = RCC_DB_FAILURE;
         delete am;
      }
   }
   else
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Two-factor authentication method \"%s\" creation failed"), name);
   }
   return rcc;
}

/**
 * Rename 2FA method
 */
uint32_t Rename2FAMethod(const TCHAR *oldName, const TCHAR *newName)
{
   return RCC_NOT_IMPLEMENTED;
}

/**
 * Deletes 2FA method
 */
uint32_t Delete2FAMethod(const TCHAR* name)
{
   uint32_t rcc = RCC_DB_FAILURE;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM two_factor_auth_methods WHERE name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
      if (DBExecute(hStmt))
      {
         s_authMethodListLock.lock();
         s_methods.remove(name);
         s_authMethodListLock.unlock();
         rcc = RCC_SUCCESS;
         NotifyClientSessions(NX_NOTIFY_2FA_METHOD_CHANGED, 0);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Check if 2FA method with given name exists
 */
bool Is2FAMethodExists(const TCHAR* name)
{
   s_authMethodListLock.lock();
   bool result = s_methods.contains(name);
   s_authMethodListLock.unlock();
   return result;
}

/**
 * Extract 2FA method binding configuration prepared for sending to client
 */
unique_ptr<StringMap> Extract2FAMethodBindingConfiguration(const TCHAR* methodName, const Config& binding)
{
   unique_ptr<StringMap> configuration;
   s_authMethodListLock.lock();
   TwoFactorAuthenticationMethod *am = s_methods.get(methodName);
   if (am != nullptr)
   {
      configuration = am->extractBindingConfiguration(binding);
   }
   s_authMethodListLock.unlock();
   return configuration;
}

/**
 * Update 2FA method binding configuration
 */
bool Update2FAMethodBindingConfiguration(const TCHAR* methodName, Config *binding, const StringMap& updates)
{
   s_authMethodListLock.lock();
   TwoFactorAuthenticationMethod *am = s_methods.get(methodName);
   if (am != nullptr)
   {
      am->updateBindingConfiguration(binding, updates);
   }
   s_authMethodListLock.unlock();
   return am != nullptr;
}

/**
 * Fills message with 2FA driver list
 */
void Get2FADrivers(NXCPMessage *msg)
{
   // Currently driver list is hard-coded
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   msg->setField(fieldId++, _T("TOTP"));
   msg->setField(fieldId++, _T("Message"));
   msg->setField(VID_DRIVER_COUNT, 2);
}
