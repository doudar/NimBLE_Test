You can easily switch between programming the server or client by switching the PlatformIO environment in vs code. 

With the current >2.2.0 NimBLE library, I cannot sucessfully get a client to subscribe to notifications from any server (esp or otherwise) with more than 3 characterists on a server. 

Feel free to go to the test server and uncomment characteristics to add or remove for testing. As soon as pFitnessService gets more than 3 Characteristics, notification callbacks on the client do not get invoked. 

Rolling NimBLE back to 2.1.3 resolves this issue. 
