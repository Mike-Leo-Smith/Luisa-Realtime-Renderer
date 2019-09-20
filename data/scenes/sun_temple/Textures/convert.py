from os import listdir, system

for file in listdir():
    if file[-4:] == ".tga":
        name = file[:-4]
        command = "convert {0}.tga {0}.bmp".format(name)
        print(command)
        system(command)