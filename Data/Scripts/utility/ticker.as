/**
 * a ticker is an integer with an upper limit which wraps around.
 *  [min,max)
 */

class WrappingTicker
{
    int value, max;
    WrappingTicker( int _excl_max )
    {
        value = 0;
        max = _excl_max;
    }

    void setMax(int _max)
    {
        max = _max > 0 ? _max : 1;
        setValue(value);
    }

    int getMax()
    {
        return max;
    }

    void setValue(int val)
    {
        while( val < 0 )
        {
            val += max;
        }
        value = val%max;
    }

    void opAssign(int val)
    {
        setValue(val);
    }

    void opPostInc()
    {
        setValue(value+1);        
    }

    void opPostDec()
    {
        setValue(value-1);        
    }

    int opConv() const
    {
        return value;
    }
}

class BlockingTicker
{
    int value, max;
    BlockingTicker( int _excl_max )
    {
        value = 0;
        max = _excl_max;
    }

    void setMax(int _max)
    {
        max = _max > 0 ? _max : 1;
        setValue(value);
    }

    int getMax()
    {
        return max;
    }

    void setValue(int val)
    {
        value = val;

        if( value < 0 )
        {
            value = 0;
        }
        
        if( value >= max )
        {
            value = max-1; 
        }
    }

    void opAssign(int val)
    {
        setValue(val);
    }

    void opPostInc()
    {
        setValue(value+1);        
    }

    void opPostDec()
    {
        setValue(value-1);        
    }

    int opConv() const
    {
        return value;
    }

}
