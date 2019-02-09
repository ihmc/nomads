
all: RobotTest.class RobotTest2.class

Robot.class: Robot.java
	javac Robot.java

MrRobot.class: MrRobot.java Robot.class CommandListener.class ConnectionLostListener.class
	javac MrRobot.java

MrSonar.class: MrSonar.java PointWithAge.class
	javac MrSonar.java

RobotTest.class: RobotTest.java MrRobot.class MrSonar.class
	javac RobotTest.java

RobotTest2Frame.class: RobotTest2Frame.java
	javac RobotTest2Frame.java

TestViewPanel.class: TestViewPanel.java
	javac TestViewPanel.java

TestViewPanel4.class: TestViewPanel4.java
	javac TestViewPanel4.java

RobotTest2.class: RobotTest2.java RobotTest2Frame.class MrRobot.class MrSonar.class TestViewPanel.class
	javac RobotTest2.java

RobotTest4.class: RobotTest4.java RobotTest4Frame.class MrRobot.class MrSonar.class TestViewPanel4.class
	javac RobotTest4.java

RobotTest3Frame.class: RobotTest3Frame.java
	javac RobotTest3Frame.java

RobotTest4Frame.class: RobotTest4Frame.java
	javac RobotTest4Frame.java

RobotTest3.class: RobotTest3.java RobotTest3Frame.class MrRobot.class MrSonar.class
	javac RobotTest3.java

CommandListener.class: CommandListener.java
	javac CommandListener.java

PointWithAge.class: PointWithAge.java
	javac PointWithAge.java

ConnectionLostListener.class: ConnectionLostListener.java
	javac ConnectionLostListener.java

run: RobotTest.class
	java RobotTest

run2: RobotTest2.class
	java RobotTest2

run3: RobotTest3.class
	java RobotTest3

run4: RobotTest4.class
	java RobotTest4

doc: MrRobot.java
	javadoc *.java

privdoc: MrRobot.java
	javadoc -private *.java

clean:
	rm -r resources/ *.class *.html *.css package-list
