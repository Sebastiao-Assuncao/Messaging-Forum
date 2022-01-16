import requests
import os
import shutil
import time
from bs4 import BeautifulSoup


def reset_state():
    # Delete existing USERS and GROUPS folders
    try:
        shutil.rmtree("src/server/USERS/")
        shutil.rmtree("src/server/GROUPS/")
    except OSError as e:
        print("Error: %s - %s." % (e.filename, e.strerror))

    os.mkdir("src/server/USERS")
    os.mkdir("src/server/GROUPS")


# ================ MAIN =====================

# EDIT THIS
IP = "193.136.128.103"
PORT = 58018
url = "http://tejo.tecnico.ulisboa.pt:59000/index.html"
do_reset = True

print(f"Running tests with IP: {IP}, Port: {PORT}, reset state: {do_reset}")

reset_state()

# Script sequences
UDP_scripts = list(range(1, 18))
TCP_scripts = list(range(20, 40))
ALL_scripts = UDP_scripts + TCP_scripts
Tejo_sequence = [6, 9, 10, 14, 21, 23, 24, 25, 26, 27,
                 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39]

ul = [6, 9, 14, 3, 20, 21]
post = [6, 9, 14, 23, 24, 25, 26, 30, 32, 34, 36, 38]
retrieve = [6, 9, 14, 23, 24, 25, 26, 30, 32,
            34, 36, 38, 27, 28, 29, 31, 33, 35, 37, 39]

# Run scripts -> Change the script sequence!!
for script_number in post:
    r = requests.get(url, params={'DSIP': IP,
                                  'DSPORT': PORT,
                                  'SCRIPT': script_number
                                  })
    print(f"=== SCRIPT {script_number} === ")
    stuff = r.text.replace("\\n", "").split("<br>")
    for i in range(len(stuff)):
        if "COMMAND" in stuff[i]:
            print(f"\n {stuff[i]} {stuff[i+1]}", end="")
    if (do_reset):
        reset_state()
    time.sleep(15)
