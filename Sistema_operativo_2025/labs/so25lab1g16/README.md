# Lab 1 : MyBash  -  Grupo 16  -  Sistemas Operativo 2025 

## 👤 Integrantes del grupo :  

- Arndt Jürgen Kiefer   
kiefer.arndt@mi.unc.edu.ar

- Huaman Rojas Alexander Jorge  
alexander.rojas@mi.unc.edu.ar

- Sierra Sierra Sebastian   
sebastian.sierra@mi.unc.edu.ar

- venguiarrutti sergio  
sergio.venguiarrutti@mi.unc.edu.ar


## 💻 Compilación y Ejecución

### Test
```sh
1. make test
2. make test-command
3. make test-parsing
4. make memtest
```

### Limpieza
```sh
1. make clean
```

### Ejecucion
```sh
1. make
2. ./mybash 
```

## 📃 Descripción de los archivos principales

* ### [**mybash.c**](mybash.c)

    El Archivo principal que hace el llamado central para ejecutar el programa y dar uso a todo lo desarrollado en el proyecto. Contiene el ciclo REPL (Read-Evaluate-Print Loop) que:

    - Lee continuamente la entrada del usuario

    - Invoca al parser para interpretar los comandos

    - Ejecuta los comandos a través del módulo execute

    - Maneja la finalización con CTRL-D y el comando exit

- ### [**parsing.c**](parsing.c)

    El proposito del parsing es lectura del parser para organizar argumentos, redirecciones y generar pipelines cuando es necesario. Se encarga de:.

    - Utilizar el parser.o provisto para analizar sintácticamente la entrada

    - Construir instancias de los TADs a partir del texto ingresado

    - Manejar errores de sintaxis y entradas inválidas robustamente

- ### [**command.c**](command.c)

    Creación de los TADs (Tipos Abstractos de Datos) que se usan en todos los demás archivos. Permite:

    - Creación, destrucción y manejo de argumentos y redirecciones

    - Implementación de scommand (comando simple) y pipeline (secuencia de comandos)

    - Funciones de debug (_to_string) para visualización de los TADs

- ### [**execute.c**](execute.c)

    Manejo de ejecución de comandos mediante llamadas al sistema. Implementa:

    - fork(), execvp(), open(), close(), pipe(), dup2(), wait()

    - Ejecución en foreground y background

    - Pipes entre procesos y redirecciones de E/S

    - Punto clave: Gestión precisa de file descriptors para evitar leaks

- ### [**builtin.c**](builtin.c)

    Manejo de comandos internos y externos. Se encarga de:

    - Comprobar si un comando es interno o externo

    - Ejecutar comandos internos (cd, help, exit) en el proceso padre

    - Implementar la lógica específica de cada comando interno
    
    <br>

Con estas descripciones se pueder ver que el flujo de ejecución de mybash sigue un camino modular que comienza cuando el usuario ingresa un comando: primero, mybash.c captura la entrada y la envía al parsing.c, donde se analiza y descompone en sus componentes esenciales (comandos, argumentos, redirecciones y pipes); luego, esta información se estructura con los TADs de command.c, que sirven como contenedores normalizados para los datos; despues, execute.c toma estos TADs y decide si debe ejecutar un comando interno llamando a builtin.c (como cd o exit) o si se trata de un comando externo, para lo cual coordina y gestiona la creación de procesos, redirecciones y tuberías utilizando llamadas al sistema; finalmente, los resultados se devuelven al usuario y el ciclo se reinicia, mostrando cómo cada módulo se especializa en una tarea específica pero colabora con los demás para transformar una línea de texto en una  ejecucion por el sistema operativo.


## 👥  Metodología de trabajo en equipo

Se realizó una distribución equitativa de las partes del proyecto según la dificultad de las implementaciones solicitadas, con la explicación de la cátedra y asistencia de IA. El laboratorio se dividió de la siguiente manera:

### Primera Parte - Módulo command.c
Se repartieron las implementaciones entre los 4 integrantes, permitiendo que cada uno se familiarizara rápidamente con las funciones. La distribución detallada puede verse en el archivo [tasks.md](./tasks.md). 

Se creó una rama `dev` que sirvió como base para que cada integrante creara su propia rama. Después de unir cada rama con `dev`, se ejecutó `make test-command` y cada integrante realizó las correcciones pertinentes, finalizando así el archivo `command.c`.

### Segunda Parte - Módulos Restantes
Para esta etapa se volvió a dividir las tareas de la manera más equilibrada posible, asignando a cada integrante nuevas tareas según la complejidad de cada módulo. La distribución específica de tareas puede consultarse en el archivo [tasks.md](./tasks.md), donde se detallan las asignaciones completas del proyecto.

### Proceso de Integración
Finalmente, se implementó en la rama `dev2` las ramas de cada integrante. Una vez unido todo, se aplicaron los tests correspondientes y se realizaron las correcciones de bugs y memory leaks. Después de las correcciones finales, se realizó la unión definitiva en la rama `master`.


## 🤖 Uso de asistentes de IA

Se utilizó ChatGPT y DeepSeek como herramientas de apoyo y consulta durante el desarrollo del proyecto:

- Consulta de sintaxis del lenguaje C, funciones y librerías

- Análisis del flujo de ejecución del programa y posibles optimizaciones

- Asesoramiento en sintaxis de README.md y Markdown para documentación

- Asistencia en la división de tareas del proyecto según complejidad

- Generación rápida de tests para depurar módulos sin pruebas unitarias

- Consultas de Debugging de errores y análisis de memory leaks

- Consultas sobre implementación de funcionalidades específicas (pipes, redirecciones, procesos)

- Revisión de mejoras prácticas para la estructura del código

## 📹 Enlace al video

🔗: https://drive.google.com/drive/folders/1d4A8I4-JzKFhHuV_VAo_gKsxWGe9ljpu?usp=sharing 

## Mejoras para la reentrega

- Se implementa el manejo de zombies para hacerles limpieza por medio de una funcion.
- Se corrije comandos individuales o ejecuciones de comandos incompletos tales como (ls |).
- Se implementa control de procesos en segundo plano (background).
- Se cambia wait por waitpid.
- Se modularizo el modulo builtin facilitando agregar nuevos builtin.

