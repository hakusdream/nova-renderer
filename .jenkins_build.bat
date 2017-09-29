#!/bin/bash

echo This is the build script for Jenkins to run. If you ware not Jenkins, you should not be running this script. It will not work and I will not support you.

set JAVA_HOME=E:\Program Files\Java\jdk1.8.0_121
copy /y nul mc_from_git

gradlew setup

cd src/main/java
rd /Q /S net
git clone https://dethraid:callosan1@github.com/NovaMods/Minecraft-Source.git net
cd net
git checkout chunks
cd ../../../../
mkdir jars\libraries
mkdir jars\versions\1.10\1.10-natives
xcopy /E /I "E:\Documents\nova-renderer\jars\libraries" "jars\libraries"
xcopy /E /I "E:\Documents\nova-renderer\src\main\java\mcp" "src\main\java\mcp"

gradlew build
gradlew buildDll

cd build
mkdir mc-orig
cd mc-orig
copy /y C:\Users\gold1\AppData\Roaming\.minecraft\versions\1.10\1.10.jar ./
bash -c "unzip -o 1.10.jar -d ."
xcopy /E /Y assets ..\