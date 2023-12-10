# Satdump webserver

- objectif create a small webserver to have acess to the created image via a website


# WARNING
- This is NOT a production server at best it will handle a small number of client
- It hasn't been tested for security, do not expose to the web



I am not a frontend developper!!! feel free to improve. 



# GET IT RUNNING

Open a terminal windows in the folder SatDump_webserver

Create a virtual environment

    python -m venv venv 
Activate the virtual environment (on Windows)

    venv\Scripts\activate 
Activate the virtual environment (on macOS/Linux)

    source venv/bin/activate

Install the dependancy in the venv 
    
    pip install -r requirements.txt

Run the webserver

    python3 main.py

Configuration:
- server create and tested on linux
- it assume that the relative path to setting.json file is: ../../.config/satdump/settings.json
  - This can be change by editing the function: load_settings_json() in main.py
- You can confire the server to remove the passes that are more than X days old by uncomment the appropriate line in the index function.
