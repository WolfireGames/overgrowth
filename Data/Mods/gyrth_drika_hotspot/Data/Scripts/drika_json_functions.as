int GetJSONInt(JSONValue data, string var_name, int default_value){
	if(data.isMember(var_name) && data[var_name].isInt()){
		return data[var_name].asInt();
	}else{
		return default_value;
	}
}

string GetJSONString(JSONValue data, string var_name, string default_value){
	if(data.isMember(var_name) && data[var_name].isString()){
		return data[var_name].asString();
	}else{
		return default_value;
	}
}

ivec2 GetJSONIVec2(JSONValue data, string var_name, ivec2 default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		return ivec2(data[var_name][0].asInt(), data[var_name][1].asInt());
	}else{
		return default_value;
	}
}

vec2 GetJSONVec2(JSONValue data, string var_name, vec2 default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		return vec2(data[var_name][0].asFloat(), data[var_name][1].asFloat());
	}else{
		return default_value;
	}
}

vec3 GetJSONVec3(JSONValue data, string var_name, vec3 default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		return vec3(data[var_name][0].asFloat(), data[var_name][1].asFloat(), data[var_name][2].asFloat());
	}else{
		return default_value;
	}
}

vec4 GetJSONVec4(JSONValue data, string var_name, vec4 default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		return vec4(data[var_name][0].asFloat(), data[var_name][1].asFloat(), data[var_name][2].asFloat(), data[var_name][3].asFloat());
	}else{
		return default_value;
	}
}

bool GetJSONBool(JSONValue data, string var_name, bool default_value){
	if(data.isMember(var_name) && data[var_name].isBool()){
		return data[var_name].asBool();
	}else{
		return default_value;
	}
}

float GetJSONFloat(JSONValue data, string var_name, float default_value){
	if(data.isMember(var_name) && data[var_name].isNumeric()){
		return data[var_name].asFloat();
	}else{
		return default_value;
	}
}

array<float> GetJSONFloatArray(JSONValue data, string var_name, array<float> default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		array<float> values;
		for(uint i = 0; i < data[var_name].size(); i++){
			values.insertLast(data[var_name][i].asFloat());
		}
		return values;
	}else{
		return default_value;
	}
}

array<int> GetJSONIntArray(JSONValue data, string var_name, array<int> default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		array<int> values;
		for(uint i = 0; i < data[var_name].size(); i++){
			values.insertLast(data[var_name][i].asInt());
		}
		return values;
	}else{
		return default_value;
	}
}

array<string> GetJSONStringArray(JSONValue data, string var_name, array<string> default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		array<string> values;
		for(uint i = 0; i < data[var_name].size(); i++){
			values.insertLast(data[var_name][i].asString());
		}
		return values;
	}else{
		return default_value;
	}
}

array<JSONValue> GetJSONValueArray(JSONValue data, string var_name, array<JSONValue> default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		array<JSONValue> values;
		for(uint i = 0; i < data[var_name].size(); i++){
			values.insertLast(data[var_name][i]);
		}
		return values;
	}else{
		return default_value;
	}
}

quaternion GetJSONQuaternion(JSONValue data, string var_name, quaternion default_value){
	if(data.isMember(var_name) && data[var_name].isArray()){
		return quaternion(data[var_name][0].asFloat(), data[var_name][1].asFloat(), data[var_name][2].asFloat(), data[var_name][3].asFloat());
	}else{
		return default_value;
	}
}

bool GetJSONValueAvailable(JSONValue data, string var_name){
	return data.isMember(var_name);
}
