{
    "RGB-221": {
        "channels": [
            2,
            1
        ],
        "offsets": {
            "1": 7
        },
        "expression": "ch2, ch2, ch1",
        "pre_equalize": true,
        "corrected": true
    },
    "RGB-321": {
        "channels": [
            3,
            2,
            1
        ],
        "offsets": {
            "1": 8,
            "3": 8
        },
        "expression": "(ch3 - 0.05) * 5, (ch2 - 0.05) * 6, (ch1 - 0.05) * 5",
        "corrected": true
    },
    "RGB-321-EQU": {
        "channels": [
            3,
            2,
            1
        ],
        "offsets": {
            "1": 8,
            "3": 8
        },
        "expression": "(ch3 - 0.05) * 5, (ch2 - 0.05) * 6, (ch1 - 0.05) * 5",
        "banding_correct": true,
        "equalize": true,
        "corrected": true
    },
    "RGB-441": {
        "channels": [
            4,
            1
        ],
        "offsets": {
            "1": 21
        },
        "expression": "(ch4 - 0.025) * 3.2, (ch4 - 0.025) * 3.2, ((ch1 * 0.6) - 0.025) * 3.2",
        "banding_correct": true,
        "corrected": true
    },
    "RGB-341": {
        "channels": [
            3,
            4,
            1
        ],
        "offsets": {
            "1": 24,
            "3": 24
        },
        "expression": "ch3, ch4, ch1",
        "corrected": true
    },
    "RGB-3(24)1": {
        "channels": [
            1,
            2,
            3,
            4
        ],
        "offsets": {
            "1": 26,
            "3": 25,
            "2": 17
        },
        "expression": "ch3, (ch2 * 0.43 + ch4 * 0.57), ch1",
        "pre_equalize": true,
        "corrected": true
    },
    "RGB-17.19.18": {
        "channels": [
            17,
            18,
            19
        ],
        "offsets": {
            "19": 9,
            "18": 5
        },
        "expression": "ch17, ch19, ch18",
        "normalize": true,
        "corrected": true
    },
    "RGB-9.12.11": {
        "channels": [
            9,
            12,
            11
        ],
        "offsets": {
            "9": 11
        },
        "expression": "1 - ch9, 1 - ch12, 1 - ch11",
        "pre_equalize": true,
        "corrected": true
    },
    "5-EQU": {
        "channels": [
            5
        ],
        "expression": "1 - ch5",
        "equalize": true,
        "normalize": true,
        "corrected": true,
        "banding_correct": true
    }
}