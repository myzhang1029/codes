/* Corvid code written for THINKERS Zhixing of UWC CSC.
 * Toggle a list of text.
 *
 * Copyright (C) 2020 Zhang Maiyun <myzhang1029@hotmail.com>, <myzhang20@uwcchina.org>.
 * All rights reserved.
 */
// The code in this file will load on every page of your site

// The mapping between the id of the "add" button and the ids of the "add" button,
// the multiply button, the text area, the corresponding image, and the visibility of the respective tabs.
let numberMappingTable = {
    21: [21, 20, 66, 22, true],
    15: [15, 18, 59, 23, false],
    16: [16, 19, 63, 24, false]
};

// Change the visibility of the buttons, textareas, and the images
function changeVisibilityAccordingToMap() {
    for (const id in numberMappingTable) {
        if (numberMappingTable[id][4]) {
            // Text visible, show multiply
            $w("#image" + numberMappingTable[id][0]).hide();
            $w("#image" + numberMappingTable[id][1]).show();
            $w("#text" + numberMappingTable[id][2]).expand();
            $w("#image" + numberMappingTable[id][3]).expand();
        } else {
            // Text invisible, show add
            $w("#image" + numberMappingTable[id][0]).show();
            $w("#image" + numberMappingTable[id][1]).hide();
            $w("#text" + numberMappingTable[id][2]).collapse();
            $w("#image" + numberMappingTable[id][3]).collapse();
        }
    }
}

function toggleTab(addNumber) {
    numberMappingTable[addNumber][4] = !numberMappingTable[addNumber][4];
    if (numberMappingTable[addNumber][4]) {
        // Collapsing others and opening this one
        for (const id in numberMappingTable) {
            if (id - addNumber !== 0) {
                console.log(id, addNumber);
                numberMappingTable[id][4] = false;
            }
        }
    }
    changeVisibilityAccordingToMap();
}

$w.onReady(function () {
    // Write your code here
    changeVisibilityAccordingToMap();
    $w("#image21").onClick(function () {
        toggleTab(21);
    });
    $w("#image15").onClick(function () {
        toggleTab(15);
    });
    $w("#image16").onClick(function () {
        toggleTab(16);
    });
    $w("#image20").onClick(function () {
        toggleTab(21);
    });
    $w("#image18").onClick(function () {
        toggleTab(15);
    });
    $w("#image19").onClick(function () {
        toggleTab(16);
    });
});