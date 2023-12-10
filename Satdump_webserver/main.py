from flask import Flask, render_template, send_from_directory
import os, json, shutil, re
from datetime import datetime, timedelta


app = Flask(__name__)

@app.route('/<path:filename>')
def image_static(filename):
    return send_from_directory(image_folder_path, filename)


def load_settings_json(setting_json_path="../../.config/satdump/settings.json"):
    """
    Load satdump_cfg.json file and return the default output directory
    :return: file path
    """
    try:
        with open(setting_json_path, 'r') as f_in:

            # Read lines and filter out lines containing ' //'
            lines = [line.split(' //')[0] for line in f_in]

            # Concatenate lines into a single string
            json_content = ''.join(lines)

            # Replace true and false by "true", "false
            json_content.replace('true', '"true"')
            json_content.replace('false', '"false"')

            # Load JSON from the processed content
            setting_json  = json.loads(json_content)

            return setting_json["satdump_directories"]["default_output_directory"]["value"]

    except FileNotFoundError:
        print("Unable to find setting.json")
        return None


# set the image folder path
image_folder_path = load_settings_json() + '/'


def explore_directory(static_path, dir):
    """
    
    :param dir_path: 
    :return: list of images dict {'name': img, 'path':  img)}
    """
    images = []
    for img in os.listdir(os.path.join(static_path, dir)):

        if os.path.join(static_path, dir, img).endswith(".png"):
            images.append({'name': img, 'path': os.path.join(dir, img)})

        elif os.path.isdir(os.path.join(static_path, dir, img)):
            images = images + explore_directory(static_path, os.path.join(dir, img))

    return images

def get_images():
    """

    :return: return a list of dict for each passes
    """
    images = []
    for image_folder in os.listdir(image_folder_path):

        if os.path.isdir(os.path.join(image_folder_path, image_folder)):

            try:
                with open(os.path.join(image_folder_path, image_folder, "dataset.json")) as f_in:
                    passe_images = {'passe_info': json.load(f_in), 'pass_images': []}

            except FileNotFoundError:
                continue
            # change time to datatime obj
            passe_images['passe_info']['timestamp'] = datetime.fromtimestamp( passe_images['passe_info']['timestamp'])

            # Get the images for this passe
            passe_images["pass_images"] = explore_directory(image_folder_path, image_folder)
            images.append(passe_images)

    images = sorted(images, key=lambda x: x["passe_info"]["timestamp"], reverse=True)

    return images

def remove_old_passe(timelimitdays = 365 ):
    """
    if the passe is older than time limites in days delete the folder
    :param timelimitdays: Limit to setup in day
    :return: number of folder deleted
    """

    nb_dir_delete = 0
    now = datetime.utcnow()
    for image_folder in os.listdir(image_folder_path):

        if os.path.isdir(os.path.join(image_folder, image_folder)):

            if "dataset.json" in os.listdir(os.path.join(image_folder_path, image_folder)):
                with open(os.path.join(image_folder_path, image_folder, "dataset.json")) as f_in:
                    passe_info = json.load(f_in)
                    # change time to datatime obj
                    time_passe = datetime.fromtimestamp(passe_info['timestamp'])

                if time_passe + timedelta(days=timelimitdays) < now:
                    shutil.rmtree(os.path.join(image_folder_path, image_folder))
                    nb_dir_delete = nb_dir_delete + 1
            else:
                print('Error occure, NO "dataset.json" Check file: ', os.path.join(image_folder_path, image_folder))

    return nb_dir_delete

@app.route('/')
def index():

    # warning the line bellow will delete the passes that are more than X days old.
    # print(remove_old_passe(X), "directory remove")
    images = get_images()
    return render_template('index.html', images=images)

if __name__ == '__main__':
    app.run(debug=True, host="0.0.0.0")

