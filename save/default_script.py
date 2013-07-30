#!/usr/bin/python
import matplotlib.pyplot as plt
from matplotlib import animation
from pylab import *
from numpy import *
import pylab as P
import sys
from time import sleep
import warnings
import numpy, scipy.io

#Ouverture du fichier passe en argument
fichier=str(sys.argv[1]) 
#Creation figure
fig=plt.figure() 

#ouverture fichier passe en argument
fo = open(fichier, "r") 
#Lecture des lignes et des colonnes
ligne = (int)(fo.readline())
colonne=(int)(fo.readline()) 

#affichage du nombre de ligne et de colonne
print ligne
print colonne

# Lis le tableau du fichier, excepte les deux premieres ligne
donnees = np.loadtxt(fichier,skiprows=2)

#filtre les warning qui viennent ensuite
warnings.filterwarnings("ignore")
#Lis le fichier excepte les deux premieres ligne, en prenant en compte les # donc les time, vire toute les ligne qui ne sont pas de la taille de la premiere traitee (cest a dire time qui est de taille 2)
nbriteration=donnees.shape[0]/ligne

data_temps= np.genfromtxt(fichier,skiprows=2,comments='a',invalid_raise=False,dtype=int)

#remet les warnings apres cette fonction
warnings.resetwarnings()

data_temps=data_temps.reshape((nbriteration,2))
#ne recupere que la seconde colonne precedente (les temps)
temps= data_temps[:,1]
#if nbriteration==1 : temps= data_temps[1]
# Note that this returned a 2D array!

#diverses verifications
print donnees.shape
print temps
print nbriteration

# However, going back to 3D is easy if we know the 
# original shape of the array
donnees = donnees.reshape((nbriteration,ligne,colonne))

print donnees.shape
print donnees.shape
print donnees

nb_images = nbriteration
image = plt.imshow(donnees[0,:,:])
plt.colorbar()
k=0

def init():
   image.set_data(donnees[0,:,:])
   return image,

# animation function.  This is called sequentially
def animate(i):
    image.set_data(donnees[i,:,:])
    return image,

anim = animation.FuncAnimation(fig, animate, init_func=init,
                               frames=nbriteration, interval=100, blit=False)


P.show()
