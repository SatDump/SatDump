{
    "BW-4.10": {
        "channels": [
            4,
            10
        ],
        "expression": "min(((1 - ch4) - 0.9) * 8, ch10 < 0.03 ? 1 : ch10)",
        "corrected": true
    },
    "RGB-388": {
        "channels": [
            3,
            8
        ],
        "offsets": {
            "8": 6
        },
        "expression": "ch3, ch8, ch8",
        "corrected": true,
        "equalize": true,
        "normalize": true
    },
    "RGB-888": {
        "channels": [
            3,
            8
        ],
        "offsets": {
            "8": 6
        },
        "expression": "ch8, ch8, ch8",
        "corrected": true,
        "equalize": true,
        "normalize": true
    },
    "CH1-EQU": {
        "channels": [
            1
        ],
        "expression": "1 - ch1",
        "corrected": true,
        "equalize": true,
        "normalize": true
    }
}
