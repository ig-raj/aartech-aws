# This code will itrate all data coming from iot core on a single file
import json
import boto3

# AWS S3 Configuration
s3_client = boto3.client('s3')
BUCKET_NAME = 'dollymaam'
FILE_KEY = 'iot_data2.json'  # Path in S3

def lambda_handler(event, context):
    # Convert IoT message to NDJSON format
    new_data = {
        "timestamp": event["timestamp"],
        "temperature": event["temperature"],
        "humidity": event["humidity"]
    }

    # Read existing data from S3
    try:
        response = s3_client.get_object(Bucket=BUCKET_NAME, Key=FILE_KEY)
        existing_data = response['Body'].read().decode('utf-8')
    except s3_client.exceptions.NoSuchKey:
        existing_data = ""  # If file doesn't exist, start fresh

    # Append new line
    updated_data = existing_data + json.dumps(new_data) + "\n"

    # Write back to S3
    s3_client.put_object(Bucket=BUCKET_NAME, Key=FILE_KEY, Body=updated_data)

    return {
        'statusCode': 200,
        'body': f"Data appended to {FILE_KEY}"
    }
