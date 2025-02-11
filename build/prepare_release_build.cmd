@ECHO OFF

cd build
perl updatetag.pl
cd ..
copy build\netxms-build-tag.properties src\java-common\netxms-base\src\main\resources\ 

for /f "delims=" %%a in ('cat build/netxms-build-tag.properties ^| grep NETXMS_VERSION ^| cut -d^= -f2') do @set VERSION=%%a
echo VERSION=%VERSION%

type src\java\netxms-eclipse\Core\plugin.xml | sed -r "s,^(.*;Version) [0-9.]+(&#x0A.*)$,\1 %VERSION%\2," > plugin.xml
copy /Y plugin.xml src\java\netxms-eclipse\Core\plugin.xml
del plugin.xml

cmd /C mvn -f src/pom.xml versions:set -DnewVersion=%VERSION% -DprocessAllModules=true
cmd /C mvn -f src/client/nxmc/java/pom.xml versions:set -DnewVersion=%VERSION%

cmd /C mvn -f src/pom.xml clean install -Dmaven.test.skip=true
cmd /C mvn -f src/client/nxmc/java/pom.xml -Pdesktop clean package

cmd /C mvn -f src/pom.xml versions:revert -DprocessAllModules=true
cmd /C mvn -f src/client/nxmc/java/pom.xml versions:revert

cd sql
make -f Makefile.w32
cd ..

echo chcp 65001 > build\update_exe_version.cmd
echo rcedit %%1 --set-file-version %VERSION% --set-product-version %VERSION% --set-version-string CompanyName "Raden Solutions" --set-version-string FileDescription %%2 --set-version-string ProductName "NetXMS" --set-version-string LegalCopyright "© 2021 Raden Solutions SIA. All Rights Reserved" >> build\update_exe_version.cmd
