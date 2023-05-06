# Note: This file need to be run automaticly when startup to dynamicly update the 
# local ip address of ESP8266 and public ngrok url

# Step by step to install ngrok
# You must have ngrok account first 
# https://ngrok.com/download

import os, requests, json, time
try:
    from dotenv.main import load_dotenv
except:
    os.system("pip install python-dotenv")
    from dotenv.main import load_dotenv

load_dotenv()    

PASSWORD = os.environ["PASSWORD"]

#Kill all ngrok session that currently running
os.system("killall ngrok")
time.sleep(1)
ip = requests.get(f"http://dqhuydev.pythonanywhere.com/get_ip?pw={PASSWORD}").text

# Start new ngrok seesion run in background
os.system(f"ngrok http {ip} --log=stdout > ngrok.log &")
time.sleep(1)

# Get url from ngrok api
response = requests.get("http://localhost:4040/api/tunnels")
json_obj = json.loads(response.text)
url = json_obj['tunnels'][0]['public_url']

# Set new url to pythonanywhere server
set_url = requests.get(f"http://dqhuydev.pythonanywhere.com/set_url?url={url}&pw={PASSWORD}")