import subprocess
import sys
import os

betsyPath = "betsy.exe"
targetDir = "./"
recordResults = False
updateResults = False
testsFailed = 0

def recordResult(filename, data):
    if not os.path.exists(filename):
        print("[REC] " + filename)
        os.makedirs(os.path.dirname(filename), exist_ok=True)
        with open(filename, "wb") as outfile:
            outfile.write(data)

def updateResult(filename, data):
    print("[UPDATE] " + filename)
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, "wb") as outfile:
        outfile.write(data)

def testResult(filename, data):
    global testsFailed
    with open(filename, "rb") as infile:
        content = infile.read()
        if data != content:
            testsFailed = testsFailed + 1
            print("[FAILED] " + filename)
            print("-EXPECTED-")
            print(content.decode("latin-1"))
            print("-ACTUAL-")
            print(data.decode("latin-1"))

def simulateTest(root, file):
    proc = subprocess.Popen([betsyPath, "sim", root + "/" + file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()

    resultDir = root + "/results_sim/"
    if recordResults:
        recordResult(resultDir + name + ".stdout", stdout)
        recordResult(resultDir + name + ".stderr", stderr)
    elif updateResults:
        updateResult(resultDir + name + ".stdout", stdout)
        updateResult(resultDir + name + ".stderr", stderr)
    else:
        testResult(resultDir + name + ".stdout", stdout)
        testResult(resultDir + name + ".stderr", stderr)

def compileTest(root, file):
    # Run compiler
    proc = subprocess.Popen([betsyPath, "com", root + "/" + file], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    # Compile program
    proc_cl = subprocess.Popen(["cl", "out.c"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    proc_cl.communicate()
    # Run program
    proc_p = subprocess.Popen(["out"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout_p, stderr_p = proc_p.communicate()

    stdout = stdout + b"\r\nProgram output:\r\n" + stdout_p
    stderr = stderr + b"\r\nProgram output:\r\n" + stderr_p

    resultDir = root + "/results_com/"
    if recordResults:
        recordResult(resultDir + name + ".stdout", stdout)
        recordResult(resultDir + name + ".stderr", stderr)
    elif updateResults:
        updateResult(resultDir + name + ".stdout", stdout)
        updateResult(resultDir + name + ".stderr", stderr)
    else:
        testResult(resultDir + name + ".stdout", stdout)
        testResult(resultDir + name + ".stderr", stderr)

if len(sys.argv) != 3:
    print("Usage: test.py <record|update> <directory to test>")
    exit()


if(sys.argv[1] == "record"):
    recordResults = True
elif(sys.argv[1] == "update"):
    updateResults = True
else:
    print("unrecognized command : " + sys.argv[1])
    exit()

targetDir = sys.argv[2]

for root, dirs, files in os.walk(targetDir):
    for file in files:
        # filter non betsy files
        name, _, extension = file.rpartition(".")
        if(extension != "betsy"):
            continue
        print(root + "/" + file)
        simulateTest(root, file)
        compileTest(root, file)      
    if os.path.exists("out.c"):
        os.remove("out.c")
    if os.path.exists("out.obj"):
        os.remove("out.obj")
    if os.path.exists("out.exe"):
        os.remove("out.exe")  

if not recordResults and not updateResults:
    print("")
    if testsFailed > 0:
        print( f"{testsFailed} tests failed.")
    else:
        print("All tests passed.")



