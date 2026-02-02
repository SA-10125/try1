const { spawn } = require("child_process");
const { exec } = require('child_process');
const path = require('path');


function runShatterlockEncrypt(stringToBeEncrypted, filename) {
    const shatterDir = path.join(__dirname, '../../shatter');
    const command = `gcc repo-hn.c -o repo-hn`;

    // Compile first, then run after compilation completes
    exec(command, { cwd: shatterDir }, (error, stdout, stderr) => {

        // Now spawn the compiled program
        const proc = spawn("repo-hn.exe", [], { cwd: shatterDir });

        proc.stdin.write(`${filename}\n${process.env.PWD}\nS\n${process.env.FIRSTANS}\n${process.env.SECONDANS}\n${stringToBeEncrypted}`);
        proc.stdin.end();
        
        proc.stdout.on("data", data => {
            console.log(data.toString());
        });
    });
}


function readShatterlockDecrypt(filename) {
    const shatterDir = path.join(__dirname, '../../shatter');
    const command = `gcc repo-hn.c -o repo-hn`;
    
    console.log("Starting compilation for decrypt");
    exec(command, { cwd: shatterDir }, (error, stdout, stderr) => {
        
        const proc = spawn("repo-hn.exe", [], { cwd: shatterDir });

        proc.stdin.write(`${filename}\n${process.env.PWD}\nR`);
        proc.stdin.end();

        proc.stdout.on("data", data => {
            console.log(data.toString());
        });
    });
}

module.exports = {runShatterlockEncrypt,readShatterlockDecrypt}