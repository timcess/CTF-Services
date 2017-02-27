from collections import OrderedDict
import pyqrcode
import json
import re

DB_NAME = "database.txt"

pattern_patient_information = OrderedDict({
    "token": 0,
    "info": {
        "name": "None",
        "age": 0,
        "diagnosis": "None",
        "addinfo": "None"
    }
})

def fill_information(token, patient_external_info):
    patient_information = OrderedDict(pattern_patient_information)
    patient_internal_info = {}
    for key in patient_external_info.keys():
        patient_internal_info[key] = str(patient_external_info[key][0])

    try:
        patient_information["token"] = token
        patient_information["info"] = patient_internal_info
    except TypeError:
        patient_information = None

    return patient_information


def get_internal_data():
    with open(DB_NAME) as db:
        data = db.readlines()
        db.close()
    data = [json.loads(info, object_pairs_hook=OrderedDict) for info in data]
    return data


def find_entry_by_token(data, token):
    for i in xrange(len(data)):
        reg = re.match(token, data[i]['token'])
        if reg != None:
            if reg.group() == data[i]['token']:
                return i
    return -1


def write_internal_data(data):
    with open(DB_NAME, 'w+') as db:
        for entry in data:
            json.dump(entry, db)
            db.write('\r\n')
    db.close()


def add_data(token, patient_info):
    patient_information = fill_information(token, patient_info)

    if patient_information is None:
        return False

    data = get_internal_data()
    index = find_entry_by_token(data, patient_information['token'])
    if index == -1:
        data.append(patient_information)
    else:
        data[index] = patient_information

    write_internal_data(data)

    return True


def delete_data(token):
    data = get_internal_data()
    index = find_entry_by_token(data, token)
    data = data[:index] + data[index:]
    write_internal_data(data)

def get_data(token):
    result = None
    data = get_internal_data()
    for entry in data:
        if entry["token"] == token:
            result = entry["info"]
            break

    return result

def create_image(patient_information):
    image = pyqrcode.create(json.dumps(patient_information))
