class set {
    array<string> values;

    // Insert value into the set, if the value already exists
    // return false otherwise return true.
    bool insert(string value) {
        if(contains(value)) {
            return false;
        } else {
            values.push_back(value);
            return true;
        }
    }

    bool contains(string value) {
        for(uint i = 0; i < values.size(); i++) {
            if( values[i] == value ) {
                return true;
            }
        }
        return false;
    }
}
