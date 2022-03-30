array<string> ConvertToStringArray( JSONValue jval )
{
    array<string> o;
    if( jval.type() == JSONarrayValue )
    {
        for( uint i = 0; i < jval.size(); i++ )
        {
            o.insertLast( jval[i].asString() );
        }
    }
    return o;
}

JSONValue getWithId( JSONValue arr, string id )
{
    if( arr.type() == JSONarrayValue )
    {
        for( uint i = 0; i < arr.size(); i++ )
        {
            if( arr[i]["id"].asString() == id )
            {
                return arr[i];
            }
        }
    }
    return JSONValue();
}

bool arrayContains( JSONValue arr, string str )
{
    if( arr.type() == JSONarrayValue )
    {
        for( uint i = 0; i < arr.size(); i++ )
        {
            if( arr[i].asString() == str )
            {
                return true;
            }
        }
    }
    return false; 
}

bool arrayContains( JSONValue arr, JSONValue str )
{
    if( arr.type() == JSONarrayValue && str.type() == JSONstringValue )
    {
        for( uint i = 0; i < arr.size(); i++ )
        {
            if( arr[i].asString() == str.asString() )
            {
                return true;
            }
        }
    }
    return false; 
}
