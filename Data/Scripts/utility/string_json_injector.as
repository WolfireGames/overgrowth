enum EvalState
{
    InitialScanning,
    LookingForSegmentEnd,         
}

class StringJSONInjector {
    JSONValue root;

    StringJSONInjector()
    {

    }

    void setRoot( string root_name, JSONValue _root )
    {
        root[root_name] = _root; 
    }

    string evaluate( string input )
    {
        string output;

        uint segment_start = 0;

        EvalState state = InitialScanning;

        for( uint i = 0; i < input.length(); i++ )
        {
            switch( state )
            {
            case InitialScanning: 
                if( input.substr(i,1) == "$" )
                {
                    if( input.substr(i,2) == "$$" ) 
                    {
                        output += "$";
                    }
                    else
                    {
                        segment_start = i;
                        state = LookingForSegmentEnd;
                    }
                }
                else
                {
                    output += input.substr(i,1);
                }
                break;
            case LookingForSegmentEnd:
                if( input.substr(i,1) == "$" )
                {
                    output += evaluateSegment(input.substr( segment_start+1, i-segment_start-1 ));
                    state = InitialScanning;
                }
                break; 
            }
        }

        if( state != InitialScanning )
        {
            Log(error, "Error evaluating the json string, ending in invalid state: " + input + "\n" );
        }
        return output;
    }

    string evaluateSegment( string seg )
    {
        bool capitalize = false;

        string work_seg = seg;

        if( work_seg.substr(0,1) == "[" )
        {
            for( uint i = 1; i < work_seg.length(); i++ )
            {
                if( work_seg.substr(i,1) == "c") 
                {
                    capitalize = true;
                } 
                else if( work_seg.substr(i,1) == "]" )
                {
                    work_seg = work_seg.substr(i+1);
                    break;  //Stop looping
                }
                else
                {
                    Log(warning, "Unknown flag in " + seg + "\n");
                }
            }
        }
       
        array<string> parts = work_seg.split(".");

        JSONValue curpos = root;

        for( uint i = 0; i < parts.length(); i++ )
        {
            if( curpos.isObject() )
                curpos = curpos[parts[i]];
        }

        if( curpos.isConvertibleTo( JSONstringValue ) )
        {
            work_seg = curpos.asString(); 
        }
        else
        {
            work_seg =  "(Failed)";
        }

        if( capitalize )
        {
            string t = ToUpper(work_seg.substr(0,1)) + work_seg.substr(1);
            work_seg = t;
        }

        return work_seg;
    }
}
