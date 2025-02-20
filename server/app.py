from flask import Flask, request, jsonify
from pymongo import ASCENDING, MongoClient
import pymongo 

app = Flask(__name__)
# MongoDB Connection
uri = "mongodb+srv://new_user1:deep617@cluster1.i8ag3.mongodb.net/?retryWrites=true&w=majority&appName=Cluster1"
client = MongoClient(uri)
db = client['face_auth']
collection = db['home_devices']
# Create an index on 'device_id' for faster queries
collection.create_index([("devices_id", ASCENDING)])

@app.route('/update_eqp', methods=['POST'])
def update_eqp_state():
     
    data = request.get_json()

    # Extract required fields
    ownerid = data.get("owner_id")
    devicesid = data.get("devices_id")
    houseid = data.get("house_id")
    floorid = data.get("floor_id")
    roomid = data.get("room_id")
    houseName = data.get("house_name")
    floorName = data.get("floor_name")
    roomName = data.get("room_name")
    eqpno = equipment.get("eqp_no")
    eqpname = equipment.get("eqp_name")
    eqpState = equipment.get("eqp_state")

    # Validate all required fields
    missing_fields = []
    if not ownerid: missing_fields.append("owner_id")
    if not devicesid: missing_fields.append("devices_id")
    if not houseid: missing_fields.append("house_id")
    if not floorid: missing_fields.append("floor_id")
    if not roomid: missing_fields.append("room_id")
    if not houseName: missing_fields.append("house_name")
    if not floorName: missing_fields.append("floor_name")
    if not roomName: missing_fields.append("room_name")
    if not eqpno: missing_fields.append("eqp_no")
    if not eqpState: missing_fields.append("eqp_state")
    
    # If any field is missing, return an error response
    if missing_fields:
        return jsonify({"error": "Missing required fields", "missing_fields": missing_fields}), 400

     # Validate each equipment object
    if eqpno is None or eqpname is None or eqpState is None:
        return jsonify({"error": "Each equipment must have 'eqp_no', 'eqp_name', and 'eqp_state'"}), 400

     # Update the equipment state **only if it exists**
    result = collection.update_one(
        {"devices_id": devicesid, "eqp_no": eqpno},  # Query filter
         {"$set": {"state": eqpState}}
     )
    return jsonify({"message": f"{eqpname} equipment {eqpState} successfully"}), 200



@app.route("/wifi", methods=["POST"])
def save_wifi_credentials():
    data = request.get_json()
    ssid = data.get("ssid")
    password = data.get("password")
    if not ssid or not password:
        return jsonify({"error": "SSID and password required"}), 400
    with open("wifi_credentials.txt", "w") as f:
        f.write(f"SSID:{ssid}\nPASSWORD:{password}")
    return jsonify({"message": "Wi-Fi credentials saved successfully"})

# List of relays with name and state
equipment = [
    {"eqp_no": "1","eqp_name": "Eqp 1", "eqp_state": False},
    {"eqp_no": "2","eqp_name": "Eqp 2", "eqp_state": False},
    {"eqp_no": "3","eqp_name": "Eqp 3", "eqp_state": False},
    {"eqp_no": "4","eqp_name": "Eqp 4", "eqp_state": False},
    {"eqp_no": "5","eqp_name": "Eqp 5", "eqp_state": False},
    {"eqp_no": "6","eqp_name": "Eqp 6", "eqp_state": False},
    {"eqp_no": "7","eqp_name": "Eqp 7", "eqp_state": False},
    {"eqp_no": "8","eqp_name": "Eqp 8", "eqp_state": False}
    
]

@app.route("/register_device", methods=["POST"])
def insert_devives():
    data = request.get_json()
    devicesname = data.get("devices_name")
    ownerid = data.get("owner_id")
    devicesid = data.get("devices_id")
    houseid = data.get("house_id")
    floorid = data.get("floor_id")
    roomid = data.get("room_id")
    houseName = data.get("house_name")
    floorName = data.get("floor_name")
    roomName = data.get("room_name")
    
    document = {
        "devices_name": devicesname,
        "owner_id": ownerid,
        "devices_id": devicesid,
        "house_id": houseid,
        "floor_id": floorid,
        "room_id": roomid,
        "house_name": houseName,
        "floor_name": floorName,
        "room_name": roomName,
        "equipments": equipment
    }
    result = collection.insert_one(document)
    collection.create_index([("devices_id", pymongo.ASCENDING)])
    print(f"Inserted Device ID: {result.inserted_id}")
    return jsonify({"message": "data saved successfully"})

if __name__ == "__main__":
    app.run()