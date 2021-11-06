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
        "expression": "ch3, ch2, ch1",
        "pre_equalize": true,
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
    "RGB-15.14.13": {
        "channels": [
            13,
            15,
            14
        ],
        "offsets": {
            "13": 2,
            "14": 7
        },
        "expression": "ch15, ch14, ch13",
        "normalize": true,
        "corrected": true
    }
}