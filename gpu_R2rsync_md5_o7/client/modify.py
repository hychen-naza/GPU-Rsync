# -*- coding: utf-8 -*-
"""
Created on Mon May  6 11:01:25 2019

@author: hchen657
"""

import os
import random
import string
from random import randint
import sys



seed = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()_+=- "
nbytes = 0
interval = int(sys.argv[1]) #1024,512,256,128...
opera_nbytes = 32
filename = sys.argv[2] #'testdata.txt'
new_filepath = "../../server/files/"
new_filename = new_filepath + filename
f = open(filename, 'r')
data_list = f.readlines()  
    
        
for i in range(len(data_list)):
    nbytes += len(data_list[i])
    if(nbytes > interval):
        nbytes = 0
        opera = randint(0, 2)      
        if(opera==0):
            #替换            
            new_string = ""
            for j in range(opera_nbytes):
                new_string += random.choice(seed)
            data_list[i] = data_list[i][:-opera_nbytes] + new_string + '\n'
            #print("line "+str(i+1) + " operation is replace, content is "+new_string)
        elif(opera==1):
            #删除
            #print("line "+str(i+1) + " operation is delete")
            data_list[i] = data_list[i][:-opera_nbytes] + '\n'
        elif(opera==2):  
            #增加       
            new_string = ""
            for j in range(opera_nbytes):
                new_string += random.choice(seed)
            data_list[i] = data_list[i][:-1] + new_string + '\n' 
            #print("line "+str(i+1) + " operation is add, content is "+new_string)
f.close()


nf = open(new_filename, 'w')
nf.writelines(data_list)  
nf.flush()
nf.close()

