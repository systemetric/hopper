"""
The spec file defines the output handlers for each input handler
For example: 
    {
        "log": [
            "log",
            "fulllog",
            "complog"
        ]
    }
defines the output handlers for the "log" input as "log", "fulllog", and "complog".
All output handlers listed will recieve messages sent to "log".

This allows input data to be handled in different ways depending on where is needs to to.
"""

class HandlerTypes:
    GENERIC = "gen"
    LOG = "log"
    FULL_LOG = "fulllog"
    COMPLETE_LOG = "complog"
    START_BUTTON = "start-button"

def get_handler_spec():
    return {
        HandlerTypes.LOG: [
            HandlerTypes.LOG,
            HandlerTypes.FULL_LOG,
            HandlerTypes.COMPLETE_LOG,
        ],
        HandlerTypes.START_BUTTON: [
            HandlerTypes.START_BUTTON,
        ]
    }