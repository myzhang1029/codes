/* Corvid code written for THINKERS Zhixing of UWC CSC.
 * Mimic countUp.js because WiX does not support it.
 *
 * Copyright (C) 2020 Zhang Maiyun <myzhang1029@hotmail.com>, <myzhang20@uwcchina.org>.
 * All rights reserved.
 */
// API Reference: https://www.wix.com/corvid/reference
// “Hello, World!” Example: https://www.wix.com/corvid/hello-world

//import { require } from "require";

//var countup = require("https://cdnjs.cloudflare.com/ajax/libs/countup.js/2.0.7/countUp.min.js");
$w.onReady(function () {

    // Write your JavaScript here

    // To select an element by ID use: $w("#elementID")

    // Click "Preview" to run your code
    /*const options = {
      separator: '',
      suffix: '+',
    };
    let demo = new countup.CountUp($w("#text10"), 400, options);
    if (!demo.error) {
      demo.start();
    } else {
      console.error(demo.error);
    }*/
    var influenced = 0;
    setInterval(function () {
        if (influenced < 400) {
            $w('#text10').text = (++influenced) + "+";
            $w('#text9').text = (~~(influenced/80)) + "+";
            $w('#text8').text = (~~(influenced/40)) + "+";
        }
    }, 2);
});