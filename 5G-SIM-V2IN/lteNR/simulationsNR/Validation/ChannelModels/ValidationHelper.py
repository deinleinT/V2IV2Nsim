
'''
author: Thomas Deinlein

Script for generating the expected pathloss values from ITU-Guidelines channel models RMaX and UMaX.
The script uses the same coordinates as in the omnet++ simulation and calculates the pathloss values for all distances between vehicle and receiver (solution is one meter).
The pathloss values are saved in a json-file and combined with the corresponding 3D distance value.
'''

import math
import json
import copy

# nodeb_height = 35  # 25 for UMaX, 10 UMiX, RMaX 35
ue_height = 1.5
carrierfrequency = 2.1
# building_height = 5  # 20 UMaX, not in UMiX, RMaX 5
street_wide = 20
pi = 3.14159265358979323846  # from omnet


class Coord:

    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def distance3d(self, other):
        retValueCoord = Coord(self.x - other.x, self.y - other.y, self.z - other.z)
        # squareLength
        retValue = pow(retValueCoord.x, 2) + pow(retValueCoord.y, 2) + pow(retValueCoord.z, 2) 
        return math.sqrt(retValue)
    
    def distance2d(self, other):
        return math.sqrt(math.pow(self.x - other.x, 2) + math.pow(self.y - other.y, 2))


##################################################################################################################################
def breakPointDistanceUM(nodebheight):
    return (4 * (nodebheight - 1) * (ue_height - 1) * ((carrierfrequency * 1000000000) / 299792458.0))

    
##################################################################################################################################   
def UMaAPathlossLOS(d3d, d2d):
    # calc dBP
    nodeb_height = 25.0
    dBP = breakPointDistanceUM(nodeb_height)
        
    pl1 = 28.0 + 22.0 * math.log(d3d, 10) + 20.0 * math.log(carrierfrequency, 10)
    pl2 = 40.0 * math.log(d3d, 10) + 28.0 + 20.0 * math.log(carrierfrequency, 10) - 9.0 * math.log(math.pow(dBP, 2) + math.pow((nodeb_height - ue_height), 2), 10)
    
    if 10 <= d2d and d2d <= dBP:
        return pl1
    elif dBP <= d2d and d2d <= 5000:
        return pl2
    else:
        raise RuntimeError("Error")  


def UMaAPathlossNLOS(d3d, d2d):
    building_height = 20.0
    nodeb_height = 25.0
    
    PLUmaNLOS = 161.04 - 7.1 * math.log(street_wide, 10) + 7.5 * math.log(building_height, 10) - (24.37 - 3.7 * math.pow(building_height / nodeb_height, 2)) * math.log(nodeb_height, 10) + (43.42 - 3.1 * math.log(nodeb_height, 10)) * (math.log(d3d, 10) - 3) + 20 * math.log(carrierfrequency, 10) - (3.2 * math.pow((math.log(17.625, 10)), 2) - 4.97) - 0.6 * (ue_height - 1.5)
    return max(PLUmaNLOS, UMaAPathlossLOS(d3d, d2d))


######################################################################################################################    
def UMaBPathlossLOS(d3d, d2d):
    # calc dBP
    nodeb_height = 25.0
    dBP = breakPointDistanceUM(nodeb_height)
        
    pl1 = 28.0 + 22.0 * math.log(d3d, 10) + 20.0 * math.log(carrierfrequency, 10)
    pl2 = 40.0 * math.log(d3d, 10) + 28.0 + 20.0 * math.log(carrierfrequency, 10) - 9.0 * math.log(math.pow(dBP, 2) + math.pow((nodeb_height - ue_height), 2), 10)
    
    if 10 <= d2d and d2d <= dBP:
        return pl1
    elif dBP <= d2d and d2d <= 5000:
        return pl2
    else:
        raise RuntimeError("Error") 

    
def UMaBPathlossNLOS(d3d, d2d):
    PLUmaNLOS = 13.54 + 39.08 * math.log(d3d, 10) + 20.0 * math.log(carrierfrequency, 10) - 0.6 * (ue_height - 1.5)
    return max(PLUmaNLOS, UMaBPathlossLOS(d3d, d2d))


######################################################################################################################
# only for carrierfrequencies below 6GHz    
def UMiAPathlossLOS(d3d, d2d):
    # calc dBP
    nodeb_height = 10.0
    dBP = breakPointDistanceUM(nodeb_height)
    
    pl1 = 22.0 * math.log(d3d, 10) + 28.0 + 20.0 * math.log(carrierfrequency, 10)
    pl2 = 40.0 * math.log(d3d, 10) + 28.0 + 20.0 * math.log(carrierfrequency, 10) - 9 * math.log(math.pow(dBP, 2) + math.pow(nodeb_height - ue_height, 2), 10)
        
    if 10 <= d2d and d2d < dBP:
        return pl1
    elif dBP < d2d and d2d < 5000:
        return pl2
    else:
        raise RuntimeError("Error")


# only for carrierfrequencies below 6GHz 
def UMiAPathlossNLOS(d3d, d2d):
    PLUmaNLOS = 36.7 * math.log(d3d, 10) + 22.7 + 26.0 * math.log(carrierfrequency, 10) - 0.3 * (ue_height - 1.5)
    return max(PLUmaNLOS, UMiAPathlossLOS(d3d, d2d))


#########################################################################################################################
def UMiBPathlossLOS(d3d, d2d):
    # calc dBP
    nodeb_height = 10.0
    dBP = breakPointDistanceUM(nodeb_height)
    
    pl1 = 32.4 + 21.0 * math.log(d3d, 10) + 20.0 * math.log(carrierfrequency, 10)
    pl2 = 32.4 + 40.0 * math.log(d3d, 10) + 20.0 * math.log(carrierfrequency, 10) - 9.5 * math.log(math.pow(dBP, 2) + math.pow(nodeb_height - ue_height, 2), 10)
        
    if 10 <= d2d and d2d <= dBP:
        return pl1
    elif dBP <= d2d and d2d <= 5000:
        return pl2
    else:
        raise RuntimeError("Error")


# not optional
def UMiBPathlossNLOS(d3d, d2d):
    PLUmaNLOS = 35.3 * math.log(d3d, 10) + 22.4 + 21.3 * math.log(carrierfrequency, 10) - 0.3 * (ue_height - 1.5)
    return max(PLUmaNLOS, UMiBPathlossLOS(d3d, d2d))


################################################################################################################################
def breakPointDistanceRM(nodebheight):
    return (2 * pi * nodebheight * ue_height * ((carrierfrequency * 1000000000) / 299792458.0))


#################################################################################################################################
# RMaA
def RMaAPathlossLOS(d3d, d2d):
    # calc dBP
    nodeb_height = 35.0
    building_height = 5.0   
    dBP = breakPointDistanceRM(nodeb_height)
    
    pl1 = 20.0 * math.log(40.0 * pi * d3d * carrierfrequency / 3, 10) + min(0.03 * pow(building_height, 1.72), 10) * math.log(d3d, 10) - min(0.044 * pow(building_height, 1.72), 14.77) + 0.002 * math.log(building_height, 10) * d3d
    pl2 = 20.0 * math.log(40.0 * pi * dBP * carrierfrequency / 3, 10) + min(0.03 * pow(building_height, 1.72), 10) * math.log(dBP, 10) - min(0.044 * pow(building_height, 1.72), 14.77) + 0.002 * math.log(building_height, 10) * dBP + 40.0 * math.log(d3d / dBP, 10)
    
    if 10 <= d2d and d2d <= dBP:
        return pl1
    elif dBP <= d2d and d2d <= 21000:
        return pl2
    else:
        raise RuntimeError("Error")  


def RMaAPathlossNLOS(d3d, d2d):
    nodeb_height = 35.0
    building_height = 5.0
    PLRmaNLOS = 161.04 - 7.1 * math.log(street_wide, 10) + 7.5 * math.log(building_height, 10) - (24.37 - 3.7 * math.pow(building_height / nodeb_height, 2)) * math.log(nodeb_height, 10) + (43.42 - 3.1 * math.log(nodeb_height, 10)) * (math.log(d3d, 10) - 3) + 20.0 * math.log(carrierfrequency, 10) - (3.2 * math.pow(math.log(11.75 * ue_height, 10), 2) - 4.97)
    return PLRmaNLOS


##################################################################################################################################
# RMaA
def RMaBPathlossLOS(d3d, d2d):
    # calc dBP
    return RMaAPathlossLOS(d3d, d2d) 


def RMaBPathlossNLOS(d3d, d2d):
    return RMaAPathlossNLOS(d3d, d2d)
##################################################################################################################################


if __name__ == '__main__':
    # starting coordinates of the vehicle and the base station in the Validation Scenario

    ueCoord = Coord(30.1, 1026.599999999999, ue_height)
########################################################################################################################
    resultMapUMaALOS = []
    # UMaA LOS
    bsCoord = Coord(4025.0, 1026.599999999999, 25)
    d2d = ueCoord.distance2d(bsCoord)
    d3d = ueCoord.distance3d(bsCoord)
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = UMaAPathlossLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapUMaALOS.append((ueCoordNew.distance3d(bsCoord), result))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("UMaALOS.json", 'w') as json_file:
        json.dump(resultMapUMaALOS, json_file)
    
    # UMaA NLOS
    resultMapUMaANLOS = []
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = UMaAPathlossNLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapUMaANLOS.append((ueCoordNew.distance3d(bsCoord), result))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("UMaANLOS.json", 'w') as json_file:
        json.dump(resultMapUMaANLOS, json_file)
#######################################################################################################################
# ##
    # UMaB LOS
    resultMapUMaBLOS = []
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = UMaBPathlossLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapUMaBLOS.append((ueCoordNew.distance3d(bsCoord), result))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("UMaBLOS.json", 'w') as json_file:
        json.dump(resultMapUMaBLOS, json_file)
        
    # UMaB NLOS
    resultMapUMaBNLOS = []
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = UMaBPathlossNLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapUMaBNLOS.append((ueCoordNew.distance3d(bsCoord), result))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("UMaBNLOS.json", 'w') as json_file:
        json.dump(resultMapUMaBNLOS, json_file)  
        
#########################################################################################################################        
   
# RMaA LOS
    resultMapRMaALOS = []
    bsCoord = Coord(4025.0, 1026.599999999999, 35)
    d2d = ueCoord.distance2d(bsCoord)
    d3d = ueCoord.distance3d(bsCoord)
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = RMaAPathlossLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapRMaALOS.append((float(ueCoordNew.distance3d(bsCoord)), float(result)))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("RMaALOS.json", 'w') as json_file:
        json.dump(resultMapRMaALOS, json_file)
 
# RMaA NLOS
    resultMapRMaANLOS = []
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = RMaAPathlossNLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapRMaANLOS.append((ueCoordNew.distance3d(bsCoord), result))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("RMaANLOS.json", 'w') as json_file:
        json.dump(resultMapRMaANLOS, json_file)
###################################################################################################################################
# RMaB LOS  
    resultMapRMaBLOS = []
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = RMaBPathlossLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapRMaBLOS.append((ueCoordNew.distance3d(bsCoord), result))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("RMaBLOS.json", 'w') as json_file:
        json.dump(resultMapRMaBLOS, json_file)

# RMaB NLOS  
    resultMapRMaBNLOS = []
    ueCoordNew = copy.copy(ueCoord)
    for x in range(int(ueCoord.x), 4016, 1):
        ueCoordNew.x = x
        result = RMaBPathlossNLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
        resultMapRMaBNLOS.append((ueCoordNew.distance3d(bsCoord), result))
        print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
    with open("RMaBNLOS.json", 'w') as json_file:
        json.dump(resultMapRMaBNLOS, json_file)  
        
           
#     # UMi_A LOS --> bs height 10m
#     resultMapUMiALOS = []
#     bsCoord = Coord(4025.0, 1026.599999999999, 10)
#     d2d = ueCoord.distance2d(bsCoord)
#     d3d = ueCoord.distance3d(bsCoord)
#     ueCoordNew = copy.copy(ueCoord)
#     for x in range(int(ueCoord.x), 4016, 1):
#         ueCoordNew.x = x
#         result = UMiAPathlossLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
#         resultMapUMiALOS.append((ueCoordNew.distance3d(bsCoord), result))
#         print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
#     with open("UMiALOS.json", 'w') as json_file:
#         json.dump(resultMapUMiALOS, json_file) 
#         
#     # UMi A NLOS --> bs height 10m
#     resultMapUMiANLOS = []
#     ueCoordNew = copy.copy(ueCoord)
#     for x in range(int(ueCoord.x), 4016, 1):
#         ueCoordNew.x = x
#         result = UMiAPathlossNLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
#         resultMapUMiANLOS.append((ueCoordNew.distance3d(bsCoord), result))
#         print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
#     with open("UMiANLOS.json", 'w') as json_file:
#         json.dump(resultMapUMiANLOS, json_file)     
# #######################################################################################################################
# # ##
#     # UMi_B LOS --> bs height 10m
#     resultMapUMiBLOS = []
#     ueCoordNew = copy.copy(ueCoord)
#     for x in range(int(ueCoord.x), 4016, 1):
#         ueCoordNew.x = x
#         result = UMiBPathlossLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
#         resultMapUMiBLOS.append((ueCoordNew.distance3d(bsCoord), result))
#         print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
#     with open("UMiBLOS.json", 'w') as json_file:
#         json.dump(resultMapUMaALOS, json_file)
# 
#     # UMi_B NLOS --> bs height --> 10m
#     resultMapUMiBNLOS = []
#     ueCoordNew = copy.copy(ueCoord)
#     for x in range(int(ueCoord.x), 4016, 1):
#         ueCoordNew.x = x
#         result = UMiBPathlossNLOS(ueCoordNew.distance3d(bsCoord), ueCoordNew.distance2d(bsCoord))
#         resultMapUMiBNLOS.append((ueCoordNew.distance3d(bsCoord), result))
#         print("Distance in m: " + str(ueCoordNew.distance3d(bsCoord)) + " pathloss result LOS: " + str(result))
#     with open("UMiBNLOS.json", 'w') as json_file:
#         json.dump(resultMapUMiBNLOS, json_file) 

##################################################################################################################################
     
