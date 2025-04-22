# electricity-co2-emission-simgrid

### Comment utiliser le plugin

Pour pouvoir utiliser notre plugin, vous devez récupérer tous les fichiers qui sont dans la directory Simgrid. Ces fichiers doivent être ensuite déposés dans l'installation Simgrid sur votre ordinateur. (Le fichier simgrid dans votre ordinateur devrait avoir les fichiers : contrib, doc, docs, exemples, …) (Vous pouvez faire un drag and drop et tous les fichiers à ajouter et à modifier seront mis à jour automatiquement.). Certains fichiers seront écrasés lors de l'installation du plugin (C'est normal). Une fois que tous les fichiers correctement ajoutés, vous pouvez utiliser les commandes suivantes pour compiler le plugin.
```
mkdir build
cd build
cmake ..
make
(sudo) make install
```
Maintenant que le plugin est compilé et installé, vous pouvez commencer à l'utiliser pour vos simulations. Vous pouvez aussi exécuter la suite de tests proposée dans le cadre du plugin. Pour faire ça, exécutez les commandes suivantes, toujours à l'intérieur de la build directory.
```
make tests
ctest -R s4u-emission-exec
```
Si vous souhaitez voir l'exécution de ces mêmes tests dans votre terminal, vous pouvez. À l'intérieur du fichier Simgrid se trouve un fichier Sujet_19-1. Dans ce fichier se trouve le fichier s4u-emission-exec1. Ouvrez un terminal dans ce fichier et exécutez les commandes suivantes.
```
mkdir build
cd build
cmake ..
make
./s4u-emission-exec1 ../src/emission_paltform.xml
```
Vous pouvez modifier les fichiers dans les fichiers du dossier src à l'intérieur de la directory Sujet_19-1 pour pouvoir créer vos propres simulations. <br><br>
Le plugin a besoin d'une valeur de g de CO2/kWh pour pouvoir calculer les émissions d'un système. Cette valeur est 42 g de CO2/kWh par défaut. Elle peut être changée avec la fonction ```sg_host_setCO2(hostName, Value);```. Elle peut aussi être définie lors de l'initialisation du plugin. Dans le fichier XML de votre simulation, vous pouvez ajouter la propriété ```<prop id="emission_file" value="../directory/dataFile.csv" />```.  Pour télécharger les fichiers utilisés par notre plugin, allez sur le site **[electicitymaps.com](https://portal.electricitymaps.com/datasets)**.

### Rendu du 3/03/2025

<p> Host_load est un plugin de l’outils SimGrid qui permet d’analyser la charge de travail d’un Host (ou le load de l’Host) dans le cadre d’une simulation. Il traque la façon dont les ressources sont utilisées. Il permet d’identifier quand des ressources sont sur ou sous utilisées. Donc il permet d’optimiser l’allocation des ressources. Il utilise les données de l’host qui sont fournies dans un fichier XML. Ensuite il utilise ces données dans une simulation qui à pour but de traquer l’utilisation des ressources. Il renvoie les résultats de la simulation de façon qu’ils soient interprétés. A la fin, il termine (kill) automatiquement les tâches de chaque host. Le plugin Host_energy est en partie basé sur le plugin Host_load.
</p>
<p> Host_energy est un plugin de l’outils SimGrid. Il permet de calculer et d’analyser la consommation d’énergie d’hosts en créant une simulation qui teste ces hosts sans avoir à les tester directement. Host_energy permet de tester différents états de l’host. Ces états sont :
  <ol>
	  <li>Off</li>
	  <li>Idle (fonctionnement de l’host quand il ne travaille pas)</li>
  	<li>Epsilon (utilisé pour calculer la courbe de fonctionnement de l’host)</li>
    <li>AllCores (fonctionnement maximal)</li>
  </ol>
Le plugin a une classe qui permet d’utiliser les données facilement (la classe PowerRange). 
</p>
<p> Le plugin a aussi une classe HostEnergy qui permet de traquer l’host et d’avoir le vecteur qui, à chaque pstat, associe une consommation électrique. Il traque le temps que l’host passe dans chaque état. Il multiplie cela par la consommation dans cet état, spécifiée lors de la création de l’host. Il finira par additionner les consommations dans chaque état l’une à l’autre.
</p>
<p> Pour utiliser le plugin, il faut d’abord définir les caractéristiques des hosts dans un fichier XML en spécifiant leurs consommations d’énergie. Ensuite, il faut initialiser la simulation. Enfin, on peut récupérer la consommation d’énergie en Joule avec get_consumed_energy(). Pour ce projet il nous suffira de transformer le résultat de cette fonction qui est en Joule vers W/h et ensuite de multiplier ce résultat par l’émission de CO2 dans le pays de l’host. Pour faire cela Il nous faudra rajouter un moyen de savoir dans quel pays est l’host. 
</p>
<p> Lorsque nous serons en mesure de tester le fonctionnement de notre plugin, nous allons créer différents cas de tests dans un « main » et pour différentes situations (différents fichier XML). Nous allons comparer les résultats obtenus avec ce qui a du sens et nous allons refaire les calculs sur papier lorsque c’est possible. Nous allons devoir tester différents cas pour vérifier le bon fonctionnement du plugin. Entre autres, parmi ces cas il y a des : 
  <ul>
    <li>Cas où les pays dans lequel est l’host n’a pas de données</li>
    <li>Cas où il y a des problèmes avec les données entrées</li>
    <li>Autres cas à déterminer une fois que le projet aura un peu plus avancé</li>
  </ul>
</p>

### Modification du 8/03 suite à la réunion de 5/03

<p> L’affichage dans le plugin host_energy se fait par fonction on_host_destruction(). Lorsqu’une simulation se termine, les informations … lors de la simulation sont affichées. Il sera donc nécessaire que nous modifions la fonction on_host_destruction() et la fonction on_simulation_end() pour qu’elles n’affichent plus la consommation en Joules des hosts et la consommation totale en Joules respectivement, mais qu’elles affichent les émissions de CO2 faites par les hosts et les émissions totales. 
</p>

<p> Pour automatiser les tests et pour pouvoir garantir une validation du plugin, nous allons modifier les fichiers s4u-enregy-exec.cpp et s4u-enregy-exec.tesh. Il faudra modifier s4u-enregy-exec.cpp pour qu’il utilise le plugin que nous allons créer et non le plugin host_energy. De plus, il faudra modifier s4u-enregy-exec.tesh pour qu’il puisse traiter le nouvel affichage de notre plugin. Les lignes à modifier sont les lignes 19 à 23 incluses. 
</p>

<p>Pour permettre de traiter la localisation des hosts, nous allons ajouter un argument lorsqu’ils seront définis dans le fichier XML avec la syntaxe : <br> 
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;prop id = “file title in program” value = “file path/file name.csv”/&gt; <br> 
Cette syntaxe sera utilisée à partir de la deuxième partie du projet qui porte sur la localisation des hosts.</p>


### Fichier Modifier pour faire fonctionner le pulgin

<p>Pour faire en sorte que notre plugin compile et fonctionne en tandem avec le code existant, nous avons dû modifier certains fichiers en dehors des fichiers que nous avons ajoutés (host_emission.cpp, emission.h, …). Nous avons dû modifier les fichiers CMakeLists.txt dans les exemples et MANIFEST.in dans le répertoire général pour pouvoir ajouter nos fichiers de tests (le programme cpp et l'output attendu tesh). De plus, il a fallu ajouter quelques lignes dans le fichier DefinePakage.cmake pour que notre programme soit compilé et utilisable.
</p>

