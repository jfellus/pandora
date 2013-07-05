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
data_temps= np.genfromtxt(fichier,skiprows=2,comments='a',invalid_raise=False,dtype=int)
#remet les warnings apres cette fonction
warnings.resetwarnings()
#ne recupere que la seconde colonne precedente (les temps)
temps= data_temps[:,1]
# Note that this returned a 2D array!

#divers verification
print donnees.shape
print temps
nbriteration=donnees.shape[0]/ligne
print nbriteration

# However, going back to 3D is easy if we know the 
# original shape of the array
donnees = donnees.reshape((nbriteration,ligne,colonne))

print donnees.shape
test=donnees[2,:,:]
print donnees.shape
print donnees
print donnees[3,:,:]
#print test.shape

# Just to check that they're the same...
#assert np.all(new_data == data)

#fig=plt.figure()
#plt.ion()

nb_images = nbriteration
#tableau = np.random.normal(10, 10, (nb_images, 10, 10))
image = plt.imshow(donnees[3,:,:])
plt.colorbar()

#image=fig.plot2D(new_data[2,:,:])
k=0

#while 1:
#    k=(k+1)%nbriteration    
#    print "image numero: %i"%k
#    image.set_data(new_data[k, :, :])
#    plt.draw()
#    sleep(0.1)

#nouveau test
#fig = plt.figure()
#ax = plt.axes(xlim=(0, 2), ylim=(-2, 2))
#line, = ax.plot(new_data[0,:,:])

nom_fichier_donnees=fichier+'_donnees'+'.mat'
nom_fichier_temps=fichier+'_temps'+'.mat'

description_matlab_donnees=fichier+'_donnees'
description_matlab_temps=fichier+'_temps'

scipy.io.savemat(nom_fichier_donnees, mdict={description_matlab_donnees: donnees})
scipy.io.savemat(nom_fichier_temps, mdict={description_matlab_temps: temps})

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
