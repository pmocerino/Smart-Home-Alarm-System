import json
import boto3

def lambda_handler(event, context):
    
    # General payload pattern should be {"device_id": device_id, "system": 0/1}, in this case a single topic both_directions is used, so device_id = 1 is considered as true
    
    # General solution:
    
    # Retrieving data from json fields
    # device_id = event['device_id']
    # system = event['system']
    # Topic will be 'home/doors/' + device_id + '/system'
    
    client = boto3.client('iot-data', region_name='us-east-1')
    
    # Change topic, qos and payload
    response = client.publish(
            topic='both_directions',
            qos=1,
            payload=json.dumps(event)
        )
    
    return {
        'statusCode': 200
    }