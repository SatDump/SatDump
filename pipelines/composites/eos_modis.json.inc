{
    "RGB-221": {
        "channels": [
            1,
            2
        ],
        "expression": "ch2 * 2.5, ch2 * 2.5, ch1 * 3",
        "corrected": true
    },
    "RGB-221-EQU": {
        "channels": [
            1,
            2
        ],
        "expression": "ch2, ch2, ch1",
        "equalize": true,
        "corrected": true
    },
    "RGB-121": {
        "channels": [
            1,
            2
        ],
        "expression": "ch1, ch2, ch1",
        "pre_equalize": true,
        "corrected": true
    },
    "RGB-121-EQU": {
        "channels": [
            1,
            2
        ],
        "expression": "ch1, ch2, ch1",
        "pre_equalize": true,
        "equalize": true,
        "corrected": true
    },
    "RGB-143": {
        "channels": [
            1,
            4,
            3
        ],
        "expression": "ch1, ch4, ch3",
        "white_balance": true,
        "corrected": true,
        "projected": true
    },
    "RGB-143-EQU": {
        "channels": [
            1,
            4,
            3
        ],
        "expression": "ch1, ch4, ch3",
        "white_balance": true,
        "equalize": true,
        "corrected": true
    },
    "RGB-143-EQURAW": {
        "channels": [
            1,
            4,
            3
        ],
        "expression": "ch1, ch4, ch3",
        "pre_equalize": true,
        "corrected": true
    },
    "29-EQU": {
        "channels": [
            29
        ],
        "expression": "1 - ch29",
        "equalize": true,
        "normalize": true,
        "corrected": true,
        "projected": true
    }
}