/*
 * Copyright (c) 2023 Krzysztof Karczewski
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

// Source code copied to clipboard after button clicked
const SOURCE_CODE = [
    "#include \"bigint.h\"\n\tint main(int argc, char *argv[]) {\n\tbigint_info();\n\treturn 0;\n}",
    "#include <string.h>\n#include \"bigint.h\"\n#define my_create(A) bigint_create(A, strlen(A))\nint main(int argc, char *argv[]) {\n\tbigint num = my_create(\"12345\");\n\tbigint_release(1, num);\n\treturn 0;\n}",
    "#include \"bigint.h\"\nint main(int argc, char *argv[]) {\n\tbigint var1 = bigint_create(\"123456789\", 9);\n\tbigint var2 = bigint_create(\"0b1100110011\", 12);\n\tbigint var3 = bigint_create(\"0xfffaaa555222\", 14);\n\tbigint_print(stdout, DEC, var1); puts(\"\");\n\tbigint_print(stdout, BIN, var2); puts(\"\");\n\tbigint_print(stdout, HEX, var3); puts(\"\");\n\tbigint_release(3, var1, var2, var3);\n\treturn 0;\n}",
    "#include <string.h>\n#include \"bigint.h\"\n#define my_create(A) bigint_create(A, strlen(A))\n#define my_print(A) bigint_print(stdout, HEX, A); puts(\"\");\nint main(int argc, char *argv[]) {\n\tbigint var1 = my_create(\"0xffff\");\n\tbigint var2 = my_create(\"0xffffffffffff\");\n\tbigint_not(var1);\n\tbigint_not(var2);\n\tmy_print(var1);\n\tmy_print(var2);\n\tbigint_release(2, var1, var2);\n\treturn 0;\n}",
    "#include \"bigint.h\"\nint main(int argc, char *argv[]) {\n\tlong unsigned int var_int = 123456789;\n\tbigint var_bint = bigint_convert_to_bigint((void*)&var_int, sizeof(var_int));\n\tbigint_print(stdout, DEC, var_bint); puts(\"\");\n\tbigint_release(1, var_bint);\n\treturn 0;\n}"
];

// Makes it harder for spam bots to read email
const ENCODED_EMAIL = 'a2Frcnp5c2llazEzQGdtYWlsLmNvbQ==';

// Stores current theme
var lightThemeOn = true;

//Changes website theme after clicking the button. User can choose between light and dark.
function changeTheme() {
    document.documentElement.className = (lightThemeOn ? "dark" : "light");
    document.getElementById("change-theme-button").title = (lightThemeOn ? "Enable light mode" : "Enable dark mode");
    document.getElementById("moon-picture").hidden = lightThemeOn;
    document.getElementById("sun-picture").hidden = !lightThemeOn;
    lightThemeOn = !lightThemeOn;
}

// Checks if access to clipboard is granted
function checkClipboardPermissions() {
    if(navigator.clipboard === undefined) {
        return false;
    }
    return navigator.permissions.query({ name : "write-on-clipboard" }).then((result) => {
        if (result.state == "granted" || result.state == "prompt") {
            return true;
        } else {
            return false;
        }
    });
}

// Copies source code to clipboard
function copyToClipboard(sourceCodeNo) {
    // No access to clipboard
    if(!checkClipboardPermissions()) {
        return;
    }
    navigator.clipboard.writeText(SOURCE_CODE[sourceCodeNo - 1]);
    button = document.getElementById("copy-button-" + sourceCodeNo.toString());
    // Change style and inform about code copied
    button.style.borderColor = "#1bc51e";
    button.innerHTML = "copied";
    button.style.color = "#1bc51e";
    // Freeze for 1.5 seconds
    setTimeout(function(){
        button.style.borderColor = "var(--copy-button-border-color)";
        button.innerHTML = "copy";
        button.style.color = "var(--text-color)";
    }, 1500);
}

// Initialise function. If not called, all the
// javaScript functionalities will stay hidden.
function init() {
    document.getElementById('change-theme-button').hidden = false;
    document.getElementById('contact').title = "Contact me";
    document.getElementById('contact').href = 'mailto:' + atob(ENCODED_EMAIL);
    changeTheme();
    if(checkClipboardPermissions()) {
        for(let i = 1; i <= SOURCE_CODE.length; i++) {
            document.getElementById("copy-button-" + i.toString()).hidden = false;
        }
    }
}
