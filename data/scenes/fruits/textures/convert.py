from os import listdir, system

for file in listdir():
    if file[-4:] == ".jpg":
        name = file[:-4]
        command = "convert {0}.jpg {0}.bmp".format(name)
        print(command)
        system(command)