# Arena Mode Data Format

The arena mode data format is build on a group of json files which are
interconnected.  All relationships are determined via the use of human-readable
string id's.

Each json file contains a certain type of data. The basic layout of them all is
an array of instances of that particular type.

The types that exist are the following: 

 * Actions
 * Arena Instances
 * Characters
 * Hidden States
 * Messages
 * Meta Choices
 * States
 * World Maps
 * World Map Nodes
 * World Nodes

## Type Descriptions

Following are detailed descriptions of the different types including usage.
Note that the dependency between them can be circular which means that there is
no clear chronology in explaining them, therefore some types might be
referenced by others without prior introduction.

### Actions

Actions are one of the most important types of objects there are. They are the
flow controllers of the game.

An action is triggered after a World Node has been executed to it's end.
Actions may have a conditional, evaluating whether or not the action should
execute. An action may set things such as states, hidden\_states and the next
worldnode.

An action has four components, an id, the "if" the "then" and the "else".

The id is a globally unique identifier used to reference the action.

The if contains a single or series of conditional statements which must
evaluate to true to result in the then being run.

The else is an optional clause which migh be run in case the if clause is
evaluted to false.

Following is an example action.json file containing a single action which
checks if the player has died a certain number of times and has a related
chance of failure to each death count. If evaluated to true the "then" is run
which ends the game by setting the next node to lose\_end\_game, adding the
death state to clarify the reason for the failure.

    "actions": [{
        "id" : "death_check",
        "if" : 
        [
            "and",
            {"if_result_match_any" : ["loss"]},
            {"excluding_states" : ["death"]},
            [
                "or",
                [
                    "and",
                    {"if_eq" : [ ["$profile.player_deaths$","1" ] ]},
                    {"chance" : 0.3}
                ],
                [
                    "and",
                    {"if_eq" : [ ["$profile.player_deaths$","2" ] ]},
                    {"chance" : 0.6}
                ],
                { "if_gt" : [ ["$profile.player_deaths$","2" ] ] }
            ]
        ],
        "then" :
        {
            "add_states" : ["death"],
            "set_world_node" : "lose_end_game"
        }
    }]

#### \"if\"

Boolean expressions are constructed with a series of objects, which contain a
single conditional; an array which begins with an operator and is followed by a
number of boolean expressions or a json boolean value. Any number of boolean
expressions can be nested, there is no explicit limit.

The boolean expressions supports the operators "and", "or" and "not".
Boolean operators are always the first element in an array. In the case with
"and" and "or" any number of expressions can follow, but with not there may
only be one. E.g. if we have an array of expressions which begin with "and",
all must evalute to true for the full bracket to evaluate to true.

The possible conditionals are the following: 

##### boolean
    
Process: A json true value evalutes to a true, same process goes for
false.

##### "if\_result\_match\_any"
    
Input: array of string ids

Process: Matches all parameters to the global result value returned if
any matches it evalutes to true.

####¤ "if\_has\_state"

Input: string id for state

Process: Evalutes if the current profile has the listed state, if so
evaluates to true.

##### "if\_has\_hidden\_state"

Input: string id for hidden\_state

Process: Evaluates the listed state and sees if the current active profile
has it, if so, evaluates to true.

##### "if\_eq"

Input: array of strings, string may use the JSON string injection language
to access variables from the profile, see the end of the arena meta
documentation for more information on that.

Process: If all listed strings in the array are equal in a string
comparions, evaluate to true.

##### "if\_gt"
    
Input: array with two string values, string values may be written in JSON injection language. 
Both parameters must be convertable into a real number.

Process is the first value, intepreted as a real number, is greater than the second, evaluate to true, else false.

###### "chance"

Input: Real number in the range from [0,1.0].

Process: A random value between 0 to 1.0 is generated, if it's less than
the input value, evaluate to true, else false.
    
#### "then"

The then block contains a series actions that will occur if the if is evaluated
to true. The possible actions are the following.

##### "set\_world\_node"

Input: world\_node id as string.

Process: Sets the current world\_node value to given valid id, switch
occurs after all other currently queued actions are evaluated.

##### "set\_meta\_choice"

Input: meta\_choice id as string.

Process: Sets the current meta choice to listed id, which will be executed
on a  world node with meta\_choice type.

##### "set\_message"

Input: message id as string.

Process: Sets the current message to listed id, which will be executed
on a world node with message type.

##### "set\_arena\_instance"

Input: arena\_instance id as string.

Process: Sets the current arena instance to listed id, which will be executed
on a world node with arena\_instance type.

##### "add\_states"

Input: String array of states

Process: Activates all listed states on the current profile.

##### "lose\_states"

Input: String array of states

Process: Deactivates all listed states from the current profile.

##### "add\_hidden\_states"

Input: String array of hidden states

Process: Activates all listed hidden states for the current profile.

##### "lose\_hidden\_state"

Input: String array of hidden states

Process: Deactivate all listed hidden states for the current profile.

##### "actions"

Input: array of actions or global action ids.

Process: Evaluates and executes all listed actions, just like a first level
action.

##### "add\_world\_map\_nodes"

Input: Array of world map node ids

Process: Enables all listed nodes, making them visible and selectable on the world map.

##### "remove\_world\_map\_nodes"

Input: Array of world map node ids.

Process: Disables all listed nodes, making them invisible and unselectable
on the world map.

##### "disable\_world\_map\_nodes"

Input: Array of world map node ids.

Process: Disables all world map nodes listed making them unselectable on the world map.

##### "enable\_world\_map\_nodes"

Input: Array of world map node ids.

Process: Enables all world map nodes listed. Making them selectable.

#### "else"

Else has the same structure as "then" with the difference that it will be run
in case the "if" is evaluated to false.

### Arena Instances

An Arena Instance is a specific arena battle request. it consists of three
values. Its id, level and battle. The id is a globally unique identifier used
in a World Node to reference an arena\_instance type and target\_id. The level
value is a path to the level xml which should be loaded. Finally the battle
value is the name of the specific battle which should be run from the levels
battle list. (Which is contained in the level if it's an arena level).

### Characters

Characters are a combined campaign start point and base player information
structure. It contains the primary information for a campaign.

#### Values

global\_actions is a list of actions which are triggered for all world nodes
throughout the entire campaign.

### States

States are value with a visual representation and title which are used by
actions to indicate a player state. They are used to determine which future
actions can be take by the campaign as the player proceeds.

### Hidden States

Hidden states work the same way as normal states, with the one exception that
they are not visible to the player. They are useful for storing a state which
might not need visualization or which needs to be hidden from the players
awareness.

### Messages

Messages are World Node targets, used with the target type "message". Messages
are a short snippet of text which is shown to the player, usually after a meta
choice or to indicate the results of an arena battle.

### Meta Choice

Meta choices are multiple choice questions or statements which give the player
a chance to impact the campaign. Each choice can be assigned a result value
which is then parsed by the actions listed in the world node.

### World Map

World maps are a WIP meta structure to reference a world map and clickable
nodes on its surface.

### World Node

A world node is a central binding structure in building a campaign. During
gameplay the play may be in "one" world node, which is referencing a
arena\_instance, message, meta\_choice or passthrough. When the target is done
processing all actions listed in a world node are executed. In each execution
there should be atleast one action which sets the current world\_node to a new
value. Otherwise the current world\_node will be rerun.

## JSON string injection (Runtime variable resolution)

Some strings (this is not specified, but it's planned that virutally all should
be included) may contain values which can be evaluated when rendered. An
example of this is printing the character name in a description, retrieving
the gender pronoun or getting the number of deaths for a conditional
evaluation.

Injection statements always start and end with a dollarsign $. Within the
dollar sign it's possible to refer to json objects. Currently only the profile
object is available (current active profile).

Example

    "$profile.character_name$"

It's possible to capitalize the string by prefixing it with [c], the [] is a
general flag construction which currently only supports capitalization. 

    "$[c]profile.character_name$"

No other operations are supported on the data, including arithmetics on multiple
values.


