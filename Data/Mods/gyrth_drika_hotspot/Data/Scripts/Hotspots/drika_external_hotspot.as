
void Reset() {
}

void Init() {
}

void SetParameters() {
	params.AddInt("Target Drika Hotspot", -1);
}

void Update() {

}

bool ObjectInspectorReadOnly(){
	return true;
}

void HandleEvent(string event, MovementObject @mo){
	SendMessage(event + " " + mo.GetID());
}

void SendMessage(string message){
	int target_id = params.GetInt("Target Drika Hotspot");
	if(target_id != -1 && ObjectExists(target_id)){
		Object@ hotspot_obj = ReadObjectFromID(target_id);
		hotspot_obj.ReceiveScriptMessage("drika_external_hotspot " + message + " " + hotspot.GetID());
	}
}
