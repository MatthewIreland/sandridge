package node.application.sandridgeApplication;

simple SandridgeApplication like node.application.iApplication {
 parameters:
 	string applicationID = default ("sandridgeApplication");
	bool collectTraceInfo = default (true);                 
	bool isSink = default (false);
	
	// the following are all in seconds
	int maxTimeBetweenReadings = default (100);
	int minTimeBetweenReadings = default (10);
	int maxSensorValue         = default (32000);
	int minSensorValue         = default (0);

	int sensorSeed         = default(1000);
	int timerSeed          = default(2000);
	int startupDelay       = default(30);

 gates:
 	output toCommunicationModule;
	output toSensorDeviceManager;
	input fromCommunicationModule;
	input fromSensorDeviceManager;
	input fromResourceManager;
}

