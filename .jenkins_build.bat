echo This is the build script for Jenkins to run. If you ware not Jenkins, you should not be running this script. It will not work and I will not support you.

rem Environment setup
set JAVA_HOME=E:\Program Files\Java\jdk1.8.0_121
copy /y nul mc_from_git
mkdir C:\WINDOWS\system32\config\systemprofile\AppData\Roaming\.minecraft\versions\1.10\
set MCP_LOC=C:\Users\gold1\AppData\Roaming\.minecraft

echo MCP Location: %MCP_LOC%

rem Nova setup
gradlew setup

cd src/main/java
rd /Q /S net
git clone https://dethraid:callosan1@github.com/NovaMods/Minecraft-Source.git net
cd net
git checkout chunks
cd ../../../../
mkdir jars\libraries
mkdir jars\versions\1.10\1.10-natives
xcopy /E /Y /I "E:\Documents\nova-renderer\jars\libraries" "jars\libraries"
xcopy /E /Y /I "E:\Documents\nova-renderer\src\main\java\mcp" "src\main\java\mcp"

rem Compilation
gradlew buildDll

xcopy /E /Y /I src\main\java mcp\src\minecraft
cd mcp\runtime
bin\python_mcp.exe recompile.py

rem Package into a new jar
mkdir mc-orig
cd mc-orig
copy /y C:\Users\gold1\AppData\Roaming\.minecraft\versions\1.10\1.10.jar ./
bash -c "unzip -o 1.10.jar -d ."
xcopy /E /Y assets ..\

rem 
