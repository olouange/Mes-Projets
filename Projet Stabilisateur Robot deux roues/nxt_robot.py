#!/usr/bin/env pybricks-micropython

from pybricks.hubs import NXTBrick
from pybricks.ev3devices import Motor, GyroSensor
from pybricks.parameters import Port
from pybricks.tools import wait

# Initialiser la brique NXT et les moteurs
nxt = NXTBrick()
moteur_gauche = Motor(Port.A)
moteur_droite = Motor(Port.B)

# Initialiser le capteur gyroscopique
capteur_gyro = GyroSensor(Port.S1)

# Constantes PID
Kp = 1.0
Ki = 0.0
Kd = 0.1

# Variables PID
integrale = 0
derniere_erreur = 0
dernier_temps = 0

# Angle cible pour la stabilisation
angle_cible = 0

# Boucle principale de stabilisation
while True:
    # Lecture de l'angle actuel depuis le capteur gyroscopique
    angle_actuel = capteur_gyro.angle()

    # Calcul de l'erreur
    erreur = angle_cible - angle_actuel

    # Calcul du temps écoulé
    temps_actuel = nxt.time()
    delta_temps = temps_actuel - dernier_temps
    dernier_temps = temps_actuel

    # Calcul de l'intégrale et de la dérivée
    integrale += erreur * delta_temps
    derivee = (erreur - derniere_erreur) / delta_temps if delta_temps > 0 else 0
    derniere_erreur = erreur

    # Calcul de la puissance de correction
    correction = Kp * erreur + Ki * integrale + Kd * derivee

    # Appliquer la correction aux moteurs
    moteur_gauche.run_power(int(correction))
    moteur_droite.run_power(int(correction))

    # Attendre un court instant avant de répéter la boucle
    wait(20)
