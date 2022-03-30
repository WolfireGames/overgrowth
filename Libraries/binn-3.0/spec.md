Binn Specification
==============

Format
--------
Each value is stored with 4 possible parameters:

<pre>
[type][size][count][data]
</pre>

But most are optional. Only the type parameter is used in all of them. Here is a list of used parameters for basic data types:
<pre>
boolean, null:
[type]

int, float (storage: byte, word, dword or qword):
[type][data]

string, blob:
[type][size][data]

list, object, map:
[type][size][count][data]
</pre>

Each parameter can be stored with polymorphic size:

Parameter | Size
------- | ----
[type]  | 1 or 2 bytes
[size]  | 1 or 4 bytes
[count] | 1 or 4 bytes
[data]  | n bytes


[Type]
-----
Each value is stored starting with the data type. It can use 1 or 2 bytes. The first byte is divided as follows:

<pre>
 +-------- Storage type
 |  +----- Sub-type size
 |  |  +-- Sub-type
000 0 0000
</pre>
#### Storage

The 3 most significant bits are used for the **storage type**. It has information about how many bytes the data will use. The storage type can be any of:

* No additional bytes
* 1 Byte
* Word (2 bytes, big endian)
* Dword (4 bytes, big endian)
* Qword (8 bytes, big endian)
* String (UTF-8, null terminated)
* Blob
* Container 

And the constants are:

Storage | Bits      | Hex  | Dec
------- | --------- | ---- | ---:
NOBYTES | **000** 0 0000 | 0x00 | 0
BYTE    | **001** 0 0000 | 0x20 | 32
WORD    | **010** 0 0000 | 0x40 | 64
DWORD   | **011** 0 0000 | 0x60 | 96
QWORD   | **100** 0 0000 | 0x80 | 128
STRING  | **101** 0 0000 | 0xA0 | 160
BLOB    | **110** 0 0000 | 0xC0 | 192
CONTAINER | **111** 0 0000 | 0xE0 | 224

#### Sub-type size

The next bit informs if the type uses 1 or 2 bytes.

If the bit is 0, the type uses only 1 byte, and the sub-type has 4 bits (0 to 15)
<pre>
 +-------- Storage type
 |  +----- Sub-type size
 |  |  +-- Sub-type
000 0 0000
</pre>
When the bit is 1, another byte is used for the type and the sub-type has 12 bits (up to 4096)
<pre>
 +-------- Storage type
 |  +----- Sub-type size
 |  |
000 1 0000  0000 0000
      |  Sub-type   |
      +-------------+
</pre>

#### Sub-type

Each storage can have up to 4096 sub-types. They hold what kind of value is stored in that storage space.

> **Example:** a DWORD can contain a signed integer, an unsigned integer, a single precision floating point number, and many more... even user defined types

Here are the values for basic data types, with the sub-type highlighted:

Type  | Storage | Bits     | Hex  | Dec
----- | ------- | -------- | ---- | ---:
Null  | NOBYTES | 0000 **0000** | 0x00 | 0
True  | NOBYTES | 0000 **0001** | 0x01 | 1
False | NOBYTES | 0000 **0010** | 0x02 | 2
UInt8 | BYTE    | 0010 **0000** | 0x20 | 32
Int8  | BYTE    | 0010 **0001** | 0x21 | 33
UInt16 | WORD    | 0100 **0000** | 0x40 | 64
Int16  | WORD    | 0100 **0001** | 0x41 | 65
UInt32 | DWORD   | 0110 **0000** | 0x60 | 96
Int32  | DWORD   | 0110 **0001** | 0x61 | 97
Float  | DWORD   | 0110 **0010** | 0x62 | 98
UInt64 | QWORD   | 1000 **0000** | 0x80 |128
Int64  | QWORD   | 1000 **0001** | 0x81 |129
Double | QWORD   | 1000 **0010** | 0x82 |130
Text   | STRING  | 1010 **0000** | 0xA0 |160
DateTime | STRING  | 1010 **0001** | 0xA1 |161
Date   | STRING  | 1010 **0010** | 0xA2 |162
Time   | STRING  | 1010 **0011** | 0xA3 |163
DecimalStr  | STRING  | 1010 **0100** | 0xA4 |164
Blob   | BLOB    | 1100 **0000** | 0xC0 |192
List | CONTAINER | 1110 **0000** | 0xE0 |224
Map  | CONTAINER | 1110 **0001** | 0xE1 |225
Object | CONTAINER | 1110 **0010**| 0xE2|226

### User Defined Types

An application can use a different DateTime type and store the value in a DWORD or QWORD.

>Storage = QWORD (0x80)<br/>
>Sub-type = 5 (0x05) [choose any unused]
>
>Type DateTime = (0x80 | 0x05 => 0x85)

An application can send HTML inside a Binn structure and can define a type to differ from plain text.

>Storage = STRING (0xA0)<br/>
>Sub-type = 9 (0x09) [choose any unused]
>
>Type HTML = (0xA0 | 0x09 => 0xA9)

If the sub-type is greater than 15, a new byte must be used, and the sub-type size bit must be set:

>Storage = STRING (0xA000)<br/>
>Sub-type size = (0x0100)<br/>
>Sub-type = 21 (0x0015)
>
>Type HTML = (0xA000 | 0x1000 | 0x0015 => 0xB015)

The created type parameter must be stored as big-endian.

[Size]
-------
This parameter is used in strings, blobs and containers. It can have 1 or 4 bytes.

If the first bit of size is 0, it uses only 1 byte. So when the data size is up to 127 (0x7F) bytes the size parameter will use only 1 byte.

Otherwise a 4 byte size parameter is used, with the msb 1. Leaving us with a high limit of 2 GigaBytes (0x7FFFFFFF).

Data size | Size Parameter Uses
---|--:
&lt;= 127 bytes | 1 byte
&gt; 127 bytes | 4 bytes

There is no problem if a small size is stored using 4 bytes. The reader must accept both.

For *strings*, the size parameter does not include the null terminator.

For *containers*, the size parameter includes the type parameter. It stores the size of the whole structure.

> **Note:** on versions prior to 2.0 the blobs sizes are stored only with 4 bytes and without support for 1-byte sizes.

[Count]
---------
This parameter is used only in containers to inform the number of items inside them. It can have 1 or 4 bytes, formatted exactly as the size parameter.

Count | Count Parameter Uses
---|--:
&lt;= 127 items | 1 byte
&gt; 127 items | 4 bytes


Containers
-------------

#### **List**
Lists are containers that store values one after another.

The count parameter informs the number of values inside the container.

>[123, "test", 2.5, true]

#### **Map**
Maps are associative arrays using **integer numbers** for the keys.

The keys are stored using a big-endian DWORD (4 bytes) that are read as a signed integer.

So the current limits are from INT32_MIN to INT32_MAX. But there is room for increase if needed.

The count parameter informs the number of key/value pairs inside the container.

>{**1:** 10, **5:** "the value", **7:** true}

#### **Object**
Objects are associative arrays using **text** for the keys.

The keys are not null terminated and the limit is 255 bytes long.

The keys are stored preceded by the key length using a single byte for it.

The count parameter informs the number of key/value pairs inside the container.

>{**"id":** 1, **"name":** "John", **"points":** 30.5, **"active":** true}


Limits
-------

Type | Min | Max
-----|-----|----
Integers | INT64_MIN | UINT64_MAX
Floating point numbers | IEEE 754 | 
Strings | 0 | 2 GB
Blobs | 0 | 2 GB
Containers | 4 | 2 GB

Associative Arrays

Key type | Min | Max
---------|-----|-----
Number | INT32_MIN | INT32_MAX
Text | 0 | 255 bytes

Sub-types: up to 4096 for each storage type


Example Structures
-----------------------
#### A json data such as {"hello":"world"} is serialized as:

**Binn:** (17 bytes)
<pre>
  \xE2           // [type] object (container)
  \x11           // [size] container total size
  \x01           // [count] key/value pairs
  \x05hello      // key
  \xA0           // [type] = string
  \x05           // [size]
  world\x00      // [data] (null terminated)
</pre>

#### A list of 3 integers:

**Json:**  (14 bytes)
>[123, -456, 789]

**Binn:** (11 bytes)
<pre>
  \xE0           // [type] list (container)
  \x0B           // [size] container total size
  \x03           // [count] items
  \x20           // [type] = uint8
  \x7B           // [data] (123)
  \x41           // [type] = int16
  \xFE\x38       // [data] (-456)
  \x40           // [type] = uint16
  \x03\x15       // [data] (789)
</pre>

#### A list inside a map:

**Json:**  (25 bytes)
>{1: "add", 2: [-12345, 6789]}

**Binn:** (26 bytes)
<pre>
 \xE1             // [type] map (container)
 \x1A             // [size] container total size
 \x02             // [count] key/value pairs
 \x00\x00\x00\x01 // key
 \xA0             // [type] = string
 \x03             // [size]
 add\x00          // [data] (null terminated)
 \x00\x00\x00\x02 // key
 \xE0             // [type] list (container)
 \x09             // [size] container total size
 \x02             // [count] items
 \x41             // [type] = int16
 \xCF\xC7         // [data] (-12345)
 \x40             // [type] = uint16
 \x1A\x85         // [data] (6789)
</pre>


#### A list of objects:

**Json:** (47 bytes)
>[
{"id": 1, "name": "John"},
{"id": 2, "name": "Eric"}
]

**Binn:** (43 bytes)
<pre>
 \xE0           // [type] list (container)
 \x2B           // [size] container total size
 \x02           // [count] items

 \xE2           // [type] object (container)
 \x14           // [size] container total size
 \x02           // [count] key/value pairs

 \x02id         // key
 \x20           // [type] = uint8
 \x01           // [data] (1)

 \x04name       // key
 \xA0           // [type] = string
 \x04           // [size]
 John\x00       // [data] (null terminated)

 \xE2           // [type] object (container)
 \x14           // [size] container total size
 \x02           // [count] key/value pairs

 \x02id         // key
 \x20           // [type] = uint8
 \x02           // [data] (2)

 \x04name       // key
 \xA0           // [type] = string
 \x04           // [size]
 Eric\x00       // [data] (null terminated)
</pre>

