array<string> CreateUnion( array<string> arr1, array<string> arr2 )
{
    array<string> res;
    for( uint i = 0; i < arr1.length(); i++ )
    {
        res.insertLast(arr1[i]); 
    }

    for( uint i = 0; i < arr2.length(); i++ )
    {
        if( res.find( arr2[i] ) < 0 )
        {
            res.insertLast(arr2[i]);
        }
    }
    return res;
}

bool contains(array<string> arr, string val) {
    for( uint i = 0; i < arr.size(); i++ ){
        if(arr[i] == val){
            return true;
        }
    }
    return false;
}
