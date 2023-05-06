# First you need a pythonanywhere account and setup a flask server on that

import os
try:
    from flask import Flask, request, redirect
except:
    os.system("pip install Flask")
    from flask import Flask, request, redirect

app = Flask(__name__)

# Default route that redirect user to
@app.route('/')
async def get_url():
    # Read url from url.txt file
    f = open("./flask_server/url.txt", "r")
    url = f.read()
    f.close()
    # Redirect webpage to the url from url.txt
    return redirect(url)

# Route use to write new url to url.txt
@app.route('/set_url', methods = ['GET', 'POST'])
async def set_url():
    pw = request.args.get("pw")
    f = open("./flask_server/password.txt", "r")
    f_pw = f.read()
    if pw != f_pw or pw == "":
        f.close()
        return "Wrong password"
    f.close()
    # Get url value from url parameter
    url = request.args.get("url")
    # Update new url to url.txt
    f = open("./flask_server/url.txt", "w")
    f.write(url)
    f.close()
    # Return success
    return 'OK'

# Route to set new local ip address of esp8266 when connect to any network
# IP address being publish from esp8266 using http request
@app.route('/set_ip', methods = ['GET', 'POST'])
async def set_ip():
    pw = request.args.get("pw")
    f = open("./flask_server/password.txt", "r")
    f_pw = f.read()
    if pw != f_pw or pw == "":
        f.close()
        return "Wrong password"
    f.close()
    ip = request.args.get("ip")
    f = open("./flask_server/ip.txt", "w")
    f.write(ip)
    f.close()
    return "OK"

# Route to get the local ip address of esp8266
# Being fetch by ubuntu server
@app.route('/get_ip')
async def get_ip():
    pw = request.args.get("pw")
    f = open("./flask_server/password.txt", "r")
    f_pw = f.read()
    if pw != f_pw or pw == "":
        f.close()
        return "Wrong password"
    f.close()
    f = open("./flask_server/ip.txt", "r")
    ip = f.read()
    f.close()
    return ip


