clear

export NOMADS_HOME=/home/mcarvalho/code/nomads
export SHARED_HOME=/home/mcarvalho/code/lib/shared

export CLASSPATH=.
export CLASSPATH=$NOMADS_HOME/misc/api:$CLASSPATH
export CLASSPATH=$NOMADS_HOME/manet/java:$CLASSPATH
export CLASSPATH=$SHARED_HOME/api:$CLASSPATH

cd ..
rm -rf test/*.class
javac *.java
javac test/*.java

cd test
