#!/usr/bin/python
# -*- coding: utf-8 -*-
################################################################################
# Copyright  ETIS — ENSEA, Université de Cergy-Pontoise, CNRS (1991-2014)
#promethe@ensea.fr
#
# Authors: P. Andry, J.C. Baccon, D. Bailly, A. Blanchard, S. Boucena, A. Chatty, N. Cuperlier, P. Delarboulas, P. Gaussier, 
# C. Giovannangeli, C. Grand, L. Hafemeister, C. Hasson, S.K. Hasnain, S. Hanoune, J. Hirel, A. Jauffret, C. Joulain, A. Karaouzène,  
# M. Lagarde, S. Leprêtre, M. Maillard, B. Miramond, S. Moga, G. Mostafaoui, A. Pitti, K. Prepin, M. Quoy, A. de Rengervé, A. Revel ...
#
# See more details and updates in the file AUTHORS 
#
# This software is a computer program whose purpose is to simulate neural networks and control robots or simulations.
# This software is governed by the CeCILL v2.1 license under French law and abiding by the rules of distribution of free software. 
# You can use, modify and/ or redistribute the software under the terms of the CeCILL v2.1 license as circulated by CEA, CNRS and INRIA at the following URL "http://www.cecill.info". 
# As a counterpart to the access to the source code and  rights to copy, modify and redistribute granted by the license, 
# users are provided only with a limited warranty and the software's author, the holder of the economic rights,  and the successive licensors have only limited liability. 
# In this respect, the user's attention is drawn to the risks associated with loading, using, modifying and/or developing or reproducing the software by the user in light of its specific status of free software, 
# that may mean  that it is complicated to manipulate, and that also therefore means that it is reserved for developers and experienced professionals having in-depth computer knowledge. 
# Users are therefore encouraged to load and test the software's suitability as regards their requirements in conditions enabling the security of their systems and/or data to be ensured 
# and, more generally, to use and operate it in the same conditions as regards security. 
# The fact that you are presently reading this means that you have had knowledge of the CeCILL v2.1 license and that you accept its terms.
################################################################################
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
nbriteration=donnees.shape[0]/ligne
#filtre les warning qui viennent ensuite
warnings.filterwarnings("ignore")
#Lis le fichier excepte les deux premieres ligne, en prenant en compte les # donc les time, vire toute les ligne qui ne sont pas de la taille de la premiere traitee (cest a dire time qui est de taille 2)
data_temps= np.genfromtxt(fichier,skiprows=2,comments='a',invalid_raise=False,dtype=int)
#remet les warnings apres cette fonction
warnings.resetwarnings()
#ne recupere que la seconde colonne precedente (les temps)
data_temps=data_temps.reshape((nbriteration,2))
temps= data_temps[:,1]
# Note that this returned a 2D array!

#diverses verifications
print donnees.shape
print temps
print nbriteration

# However, going back to 3D is easy if we know the 
# original shape of the array
donnees = donnees.reshape((nbriteration,ligne,colonne))

nom_fichier_donnees=fichier+'_donnees'+'.mat'
nom_fichier_temps=fichier+'_temps'+'.mat'

description_matlab_donnees=fichier+'_donnees'
description_matlab_temps=fichier+'_temps'

scipy.io.savemat(nom_fichier_donnees, mdict={description_matlab_donnees: donnees})
scipy.io.savemat(nom_fichier_temps, mdict={description_matlab_temps: temps})
