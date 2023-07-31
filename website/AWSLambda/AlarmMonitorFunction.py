import json
# import the AWS SDK
import boto3

# Funtion to sort by sample time
def sort_by_key(list):
    return list['sample_time']


# Define the handler function that the Lambda service will use
def lambda_handler(event, context):

    # Create a DynamoDB object using the AWS SDK
    dynamodb = boto3.resource('dynamodb')
    
    # Use the DynamoDB object to select tables
    system_table = dynamodb.Table('SystemTable')
    alarm_table = dynamodb.Table('AlarmTable')

    # Retrieve tuples of our tables to return
    output_system = system_table.scan()["Items"]
    output_alarm = alarm_table.scan()["Items"]
    
    # Sort by sample time
    output_system = sorted(output_system, key=sort_by_key);
    output_alarm = sorted(output_alarm, key=sort_by_key)
    
    
    return {
       'statusCode': 200,
       'body_system': output_system,
       'body_alarm': output_alarm
    }
    