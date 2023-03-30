const args = process.argv;
const eventLog = args[2];
var eventLogFolder = require('path').dirname(eventLog);
const outputPath = args[3];
const fs = require('fs');

let content = fs.readFileSync(eventLog,  'utf8')
const lines = content.split("\n")
let i = 0

console.log(`Adding start`)
fs.writeFileSync(outputPath, '[\n');

lines.forEach((line) => {
  i++
  if (i == 1) return;
  console.log("At line: ",i,"out of ", lines.length)
  line = line.split(",")
  if (line.length != 5) return
  let curr = {}
  curr["timestamp"] = parseInt(line[0]);
  curr["eventType"] = line[1];
  curr["time"] = parseInt(line[2]);
  curr["pid"] = parseInt(line[3]);
  if (line[4] !== "") {
    let fLoad = fs.readFileSync(eventLogFolder + "/" + line[4])
    let jFile = JSON.parse(fLoad);
    curr = {...curr, ...jFile}
  }
  if (i != lines.length - 1) {
    fs.appendFileSync(outputPath, JSON.stringify(curr) + ",\n");
  } else {
    fs.appendFileSync(outputPath, JSON.stringify(curr) + "\n");
  }
});

console.log(`Adding end`)
fs.appendFileSync(outputPath, ']');
process.exit(0)
