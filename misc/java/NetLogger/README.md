# NetLogger
NetLogger is a pure Java/JavaFX application used to display incoming data in a visibly pleasing way. Data is received after subscribing to a [NATS](https://nats.io) server. Data sent via NATS is serialized in Google's [Protocol buffers](https://developers.google.com/protocol-buffers/) in the Measure format, a custom protobuf format created here at IHMC.

## Running

### Executable only
If you just want to run NetLogger, you may go into the root directory and type (`./gradlew installDist`). This creates a batch file under `$rootDir\build\install\netlogger\bin` to launch NetLogger. Within this batch file you may also change parameters on the lines that start with `set CMD_LINE_ARGS=`. 

### Source code with IntelliJ
NetLogger was built using Gradle for dependency management. The only known version of Gradle to work is Gradle 4.1. You may open NetLogger using IntelliJ. Open IntelliJ and choose "Open". Then navigate to NetLogger's root directory and double click on the folder (it may have the green Gradle icon instead of a regular folder). You will be prompted to configure some things.

Select:  

 - [x] Use auto-import
 - [x] Create separate module per source set
 - [x] Use local gradle distribution
  **OR**
 - [x] Use gradle 'wrapper' task configuration

Be sure to set your Gradle home to your Gradle installation if it does not default it for you.
For the Gradle JVM, you should set that to your local JDK version. JDK 1.8.0_151-162 have been tested. Now you should be able to find the code of NetLogger under `src/main/java`. The main class to launch the application is `Launcher`. 

If you do not have a NATS server to connect to, you will still be able to see NetLogger in action by going to the `Settings > General` and choosing `play test`.