# AWSLambda
This folder contains Python scripts used for AWS Lambda functions. 

In detail, `AlarmMonitorFunction.py` retrieves and returns data from the two DynamoDB tables, called SystemTable and AlarmTable.
The function `SystemHandlerFunction.py`, instead, is associated to the POST method of deployed AWS API, in order to publish the information given by the activation/deactvation button to local MQTT broker, through the bridge. In this way, when the input from dashboard button is obtained, the MCU can know whether to activate or deactivate through the subscription to a local MQTT topic.
Finally, the function `CognitoAutoConfirm.py` is used in order to auto-confirm user and e-mail (and possibly phone number, if specified in AWS Cognito User pool, but not in my case). In fact, I encountered some issues with mail reception from AWS, because mails were never sent to my e-mail address, hence I couldn't insert verification code to verify my mail. With this function, the e-mail is automatically verified.
