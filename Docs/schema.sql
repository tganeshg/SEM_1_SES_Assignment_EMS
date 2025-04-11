//Create table
CREATE TABLE IF NOT EXISTS SensorData (	ID INTEGER PRIMARY KEY AUTOINCREMENT, 
										Device_ID INTEGER,
										Timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
										Power_Consumption INTEGER);

//Insert data
INSERT INTO SensorData (Device_ID, Power_Consumption) VALUES (val1, val2);

//Delete the data if beyond one day for Main process
DELETE FROM SensorData WHERE Timestamp < datetime('now', '-1 day');

//Get the data to publish the data to cloud last one min data
SELECT Device_ID, Power_Consumption, Timestamp FROM SensorData WHERE Timestamp >= datetime('now', '-60 seconds');

//Get the data for Live data
SELECT sensorID, power, timestamp FROM SensorData 
				 WHERE timestamp = (SELECT MAX(timestamp) FROM SensorData WHERE sensorID = SensorData.sensorID)

//Get the data for one hour average
SELECT sensorID, AVG(power) as avg_power, strftime('%Y-%m-%d %H:00:00', timestamp) 
											as hour FROM SensorData GROUP BY sensorID, hour