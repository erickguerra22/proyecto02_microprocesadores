/*---------------------------------------------------------------------------
* UNIVERSIDAD DEL VALLE DE GUATEMALA
* FACULTAD DE INGENIERi
* DEPARTAMENTO DE CIENCIA DE LA COMPUTACIoN
*
* Curso:       CC3086 - Programacion de microprocesadores     Ciclo II - 2022
* Descripcion: Proyecto 2(Laboratorio 5)/ Cajero automatico hecho con PThreads
* Fecha de creacion: 7/09/2022
* ultima modificacion: 10/10/2022
* Autores:       Herber Sebastian Silva Munoz - 21764
*                Daniel Esteban Morales Urizar - 21785
*                Erick Stiv Junior Guerra Munoz - 21781
------------------------------------------------------------------------------*/
// Importacion de librerias y dependencias
#ifdef _WIN32
#include <Windows.h>
#endif
#include <cstdlib>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
//#include <string>
#include <semaphore.h>
#define NUMTHREADS 20
#define NUMUSERS 20

using namespace std;

// Declaracion de variables globales
int usersCount = 5;
int opRegistradas = 0;
float atmAmount = 20000.0;

// Estructura de instancia de usuario
struct user
{
    string name;
    string lastname;
    string mail;
    string password;
    int account;
    string userCode;
    float amount;
    float pendingAmount;
};

// Estructura de las operaciones a realizar
struct operation{
    int id;
    int type; //100: retiro, 200: pago, 300: prestamo, 400: transferencia
    float amount;
    long destAccount;
    long originAccount;
    string message;
};

// Lista de usuarios preestablecida
user userList[NUMUSERS] = {
    {"Sebastian", "Silva", "ssilva123@gmail.com", "admin", 831547925, "9176", 5840.15, 300.00},
    {"Daniel", "Morales", "dmorales456@gmail.com", "admin", 461554984, "1651", 3695.38, 0.00},
    {"Erick", "Guerra", "eguerra789@gmail.com", "admin", 451384651, "7965", 4613.7, 560.00},
    {"Gabriela", "Rodriguez", "grodriguez987@gmail.com", "grodriguez", 916458732, "3627", 987.4, 200.00},
    {"Jane", "Asher", "jasher654@gmail.com", "jasher", 503179521, "5048", 9865.27, 3000.00}};

pthread_mutex_t mtx;
sem_t semaforo;

// Instancia nula (default) e inicializacion del usuario logeado.
user defaultUser = {"", "", "", "", 0, "", 0, 0};
user currentUser = defaultUser;

// Funcion encargada de mostrar un menu al usuario y verificar que este seleccione una opcion valida.
int readOption(string menu, int opciones)
{
    int op = 0;
    bool repeat = true;
    while (repeat)
    {
        cout << menu << endl;
        cin >> op;
        if (op > 0 && op <= opciones)
            repeat = false;
        else
            cout << "Opcion no valida" << endl;
    }
    return op;
}

// Funcion encargada de iniciar sesion en el programa
bool login(string usuario, string password)
{
    currentUser = defaultUser;
    for (user u : userList)
    {
        if ((u.mail == usuario || u.userCode == usuario) && u.password == password)
            currentUser = u;
    }
    return (currentUser.name == defaultUser.name) ? false : true;
}

// Funcion encargada de generar un numero aleatorio de longitud especifica
int generateNumber(int length)
{
    int infLim = 1 * (pow(10, length - 1));
    int supLim = 1 * (pow(10, length)) - infLim;
    int number = infLim + rand() % supLim;

    return number;
}

// Funcion que valida la creacion de un numero de cuenta o de codigo de acceso validos
int validateInfo(int number, int type)
{ // type: 0 - usuario, 1- cuenta
    for (user u : userList)
    {
        if (type == 0 && to_string(number) == u.userCode)
            number = validateInfo(generateNumber(4), 0);
        if (type == 1 && number == u.account)
            number = validateInfo(generateNumber(9), 1);
    }
    return number;
}

// Funcion encargada de registrar un nuevo usuario en el programa
string registro(string name, string lastname, string mail, string password, float amount)
{
    currentUser = defaultUser;
    bool usuario_existente = false;
    for (user u : userList)
    {
        if (u.mail == mail)
            usuario_existente = true;
    }
    if (!usuario_existente)
    {
        if (usersCount < NUMUSERS)
        {
            user new_user = {name, lastname, mail, password, validateInfo(generateNumber(9), 1), to_string(validateInfo(generateNumber(4), 0)), amount, 0};
            userList[usersCount] = new_user;
            usersCount++;
            currentUser = new_user;
            string retorno = "Numero de cuenta: " + to_string(new_user.account) + "\nCodigo de seguridad: " + new_user.userCode + "\nCuenta creada exitosamente, ahora puede iniciar sesion.";
            Sleep(50);
            return retorno;
        }
        else
            return "Ya no se permite el registro de mas usuarios.";
    }
    else
    {
        return "El correo electronico especificado ya esta en uso";
    }
}

// Funcion que valida que la cuenta de destino exista
bool validateAccount(int account)
{
    bool flag = false;
    for (user u : userList)
    {
        if (account == u.account)
            flag = true;
    }
    return flag;
}

// Funcion que verifica que el cajero posee los fondos necesarios en efectivo para hacer el retiro esperado
bool isMoney(int amount)
{
    bool flag = false;
    if (atmAmount >= amount)
    {
        flag = true;
    }
    return flag;
}

// Subrutina que hace un retiro del cajero, a la vez que actualiza el dinero disponible del usuario.
void *retiro(void *args)
{
    operation *ps;
    ps = (operation *)args;
    printf("Realizando operacion %d: retiro.\n",(ps->id));

    sem_wait(&semaforo);
    if(isMoney(ps->amount) == false)
        (ps->message) = "\nEl cajero no posee suficiente dinero en estos momentos, por favor, vuelva mas tarde";
    else if(ps->amount <= 0)
        (ps->message) = "\nLa cantidad indicada para retirar no es valida.\n";
    else{
        for (int i = 0; i < NUMUSERS; i++)
        {
            if ((ps->originAccount) == userList[i].account)
            {
                pthread_mutex_lock(&mtx);
                atmAmount = atmAmount - (ps->amount);
                userList[i].amount = userList[i].amount - (ps->amount);
                pthread_mutex_unlock(&mtx);
                (ps->message) = "\n--------------------------------------------------\nTome su dinero...\nEstimad@ " + userList[i].name + " le quedan: " + to_string(userList[i].amount) + " quetzales en su cuenta\n--------------------------------------------------";
                sem_post(&semaforo);
            }
        }
    }
    printf("%s\n\n",ps->message.c_str());
}

// Subrutina que realiza un pago al banco, a la vez que actualiza la deuda del usuario.
void *pago(void *args)
{
    operation *ps;
    ps = (operation *)args;
    printf("Realizando operacion %d: pago.\n",ps->id);

    sem_wait(&semaforo);
    if(ps->amount <= 0){
        (ps->message) = "Error: La cantidad ingresada no es valida.\n";
    }else{
        for (int i = 0; i < NUMUSERS; i++)
        {
            if ((ps->originAccount) == userList[i].account)
            {
                pthread_mutex_lock(&mtx);
                if(ps->amount > userList[i].pendingAmount){

                }else{
                    (ps->message) = "Error: Esta intentando abonar una cantidad mayor a la que debe.";
                }
                    atmAmount = atmAmount + (ps->amount);
                    userList[i].amount = userList[i].amount - (ps->amount);
                    userList[i].pendingAmount = userList[i].pendingAmount - (ps->amount);
                    pthread_mutex_unlock(&mtx);
                    (ps->message) = "\n--------------------------------------------------\nPago realizado con exito, su nueva deuda es de " + to_string(userList[i].pendingAmount) +"\nEstimad@ "+ userList[i].name + ", le quedan: " + to_string(userList[i].amount) + " quetzales en su cuenta\n--------------------------------------------------";
                    sem_post(&semaforo);
            }
        }
    }
    printf("%s\n\n",ps->message.c_str());
}

// Subrutina que solicita un prestamo al banco, a la vez que actualiza los fondos del un usuario.
void *prestamo(void *args)
{
    operation *ps;
    ps = (operation *)args;
    printf("Realizando operacion %d: prestamo.\n",ps->id);

    sem_wait(&semaforo);
    if(isMoney(ps->amount) == false)
        (ps->message) = "\nEl cajero no posee suficiente dinero en estos momentos, por favor, vuelva mas tarde.\n";
    else if(ps->amount <= 0)
        (ps->message) = "\nEl monto indicado no es valido.\n";
    else{
        for (int i = 0; i < NUMUSERS; i++)
        {
            if ((ps->originAccount) == userList[i].account)
            {
                pthread_mutex_lock(&mtx);
                atmAmount = atmAmount - (ps->amount);
                userList[i].amount = userList[i].amount + (ps->amount);
                userList[i].pendingAmount = userList[i].pendingAmount + (ps->amount);
                pthread_mutex_unlock(&mtx);
                (ps->message) = "\n--------------------------------------------------\nPrestamo debitado exitosamente.\nEstimad@ " + userList[i].name + " su nuevo saldo disponible es de: " + to_string(userList[i].amount) + "\n--------------------------------------------------";
                sem_post(&semaforo);
            }
        }
    }
    printf("%s\n\n",ps->message.c_str());
}

// Subrutina que realiza la transferencia de fondos entre usuarios.
void *transf(void *args)
{
    operation *ps;
    ps = (operation *)args;
    printf("Realizando operacion %d: transferencia.\n",ps->id);

    sem_wait(&semaforo);
    if(validateAccount(ps->destAccount) == false)
        (ps->message) = "\nError: La cuenta destino especificada no existe.\n";
    else if(ps->amount <= 0)
        (ps->message) = "\nEl monto indicado no es valido.\n";
    else{
        for (int i = 0; i < NUMUSERS; i++)
        {
            if ((ps->originAccount) == userList[i].account)
            {
                for(int j = 0; j<NUMUSERS;j++){
                    if((ps->destAccount) == userList[j].account){

                        pthread_mutex_lock(&mtx);
                        userList[j].amount = userList[j].amount + (ps->amount);
                        userList[i].amount = userList[i].amount - (ps->amount);
                        pthread_mutex_unlock(&mtx);
                        (ps->message) = "\n--------------------------------------------------\nDinero transferido correctamente a la cuenta de " +userList[j].name + " " + userList[j].lastname + "\nEstimad@ " + userList[i].name + ", le quedan " + to_string(userList[i].amount) + " quetzales en su cuenta\n--------------------------------------------------";
                        sem_post(&semaforo);
                    }
                }
            }
        }
    }
    printf("%s\n\n",ps->message.c_str());
}

// Funcion principal
int main()
{
    // Variables locales
    int opcion = 0;
    bool repeatAll = true;
    bool repeatSession = true;
    pthread_t hilos[NUMTHREADS];
    string *results[NUMTHREADS];

    // Menu de inicio
    string menu1 = "\n-Seleccione una opcion-\n1. Iniciar Sesion\n2. Registrarse\n------------------------------------------------\n";

    // Menu principal del programa
    string menu2 = "\n----------------------------------------------------\nPor favor, selecciona una opcion:\n";
    menu2 = menu2 + "1. Retiro de efectivo\n";
    menu2 = menu2 + "2. Pago de saldo pendiente\n";
    menu2 = menu2 + "3. Solicitud de prestamo\n";
    menu2 = menu2 + "4. Consulta de saldo\n";
    menu2 = menu2 + "5. Transferencia a otro usuario\n";
    menu2 = menu2 + "6. Salir\n";
    menu2 = menu2 + "------------------------------------------------------\n";

    //string menu2 = "\n------------------------------------------------\nPor favor, selecciona una opcion:\n1. Retiro de efectivo\n2. Transferencia a otro usuario\n3. Salir\n------------------------------------------------\n";

    // Menu de repeticion
    string menuFinal = "\n--------------------------------------------------\nDesea realizar otra accion?\n1. Si\n2. No\n--------------------------------------------------\n";

    // Este bloque se ejecutara siempre que la variable repeatAll sea true
    while (repeatAll)
    {
        cout << "\n------------------------------------------------\nBienvenido al sistema de cajeros HorizonBank";
        opcion = readOption(menu1, 2);
        /*string password = "";
        string name = "";
        string user = " ";
        string lastname = "";
        string mail = "";*/
        switch (opcion)
        {

            // En caso de seleccionar Login
            case 1:{
                string user = " ";
                string password = " ";
                cout << "\nIngrese su correo electronico o codigo de seguridad: ";
                cin >> user;
                cout << "\nIngrese su contrasena: ";
                cin >> password;
                if (login(user, password))
                {
                    cout << "\nSesion iniciada correctamente" << endl;
                    repeatSession = true;
                    break;
                }
                else
                    cout << "Credenciales incorrectas, intente de nuevo o registrese" << endl;
                break;
            }

            // En caso de seleccionar registro
            case 2:{
                string name = "";
                string lastname = "";
                string mail = "";
                string password = "";
                float amount = 0;
                cout << "Ingresa la informacion que se solicita a continuacion:" << endl;
                cout << "Nombre: ";
                cin >> name;
                cout << "Apellido: ";
                cin >> lastname;
                cout << "Correo electronico: ";
                cin >> mail;
                cout << "Contrasena: ";
                cin >> password;
                cout << "Cantidad actual de la cuenta: ";
                cin >> amount;
                cout << registro(name, lastname, mail, password, amount) << endl;
                break;
            }
        }

        // Si el usuario ya esta logeado
        if (currentUser.name != defaultUser.name)
        {
            operation operationList[NUMTHREADS];
            opRegistradas = 0;
            cout << "Que bueno verte, " << currentUser.name << "!" << endl;
            while (repeatSession)
            {
                //Mostrar menu de opciones secundarias
                opcion = readOption(menu2, 6);
                switch (opcion)
                {
                    //En caso de retiro    
                    case 1:{
                        float retiro = 0;
                        cout << "Ingrese la cantidad que desea retirar" << endl;
                        cin >> retiro;
                        if(retiro > currentUser.amount)
                            cout << "Error: La cantidad que desea retirar excede la existente en su cuenta.";
                        else{
                            opRegistradas = opRegistradas +1;
                            operationList[opRegistradas-1] = {opRegistradas, 100, retiro, 0, currentUser.account,""};
                            cout << "Operacion agregada a la cola exitosamente." << endl;
                        }
                        break;
                    }
                    case 2:{
                        float pendiente = currentUser.pendingAmount;
                        float pago = 0;
                        cout << "Actualmente debe " << pendiente << " quetzales al banco" << endl;
                        cout << "Ingrese la cantidad que desea abonar: " << endl;
                        cin >> pago;
                        if(pendiente == 0)
                            cout << "Operacion no valida: Actualmente no debe dinero al banco." << endl;
                        else if(pago > currentUser.amount)
                            cout << "No cuenta con dinero suficiente para realizar el pago indicado.";
                        else{
                            opRegistradas = opRegistradas + 1;
                            operationList[opRegistradas-1] = {opRegistradas, 200, pago, 0, currentUser.account,""};
                            cout << "Operacion agregada a la cola exitosamente." << endl;
                        }
                        break;
                    }
                    case 3:{
                        float monto = 0;
                        cout << "Ingrese la cantidad que desea solicitar al banco: "<< endl;
                        cin >> monto;
                        if(currentUser.pendingAmount > 200)
                            cout << "Error: Actualmente cuenta con un pago pendiente que excede el limite de 200 quetzales permitido por el banco, realice el pago correspondiente antes de solicitar otro prestamo." << endl;
                        else{
                            opRegistradas = opRegistradas + 1;
                            operationList[opRegistradas-1] = {opRegistradas, 300, monto, 0, currentUser.account,""};
                            cout << "Operacion agregada a la cola exitosamente." << endl;
                        }
                        break;
                    }
                    case 4:{
                        cout << "El saldo actual de su cuenta es: " << currentUser.amount << endl;
                        cout << "La cantidad pendiente por cancelar es: " << currentUser.pendingAmount << endl;
                        break;
                    }
                    
                    //En caso de transferencia 
                    case 5:{
                        float monto = 0;
                        long destAccount = 0;
                        cout << "Ingrese la cuenta a la que desea realizar la transferencia: " << endl;
                        cin >> destAccount;
                        cout << "Ingrese el monto a transferir: " << endl;
                        cin >> monto;
                        if(monto > currentUser.amount)
                            cout << "Error: La cantidad que desea transferir excede la disponibilidad actual de su cuenta." << endl;
                        else{
                            opRegistradas = opRegistradas + 1;
                            operationList[opRegistradas-1] = {opRegistradas, 400, monto, destAccount, currentUser.account,""};
                            cout << "Operacion agregada a la cola exitosamente." << endl;
                        }
                        break;
                    }

                    //En caso de cerrar sesion 
                    case 6:{
                        cout << "Ha cerrado su sesion" << endl;
                        repeatSession = false;
                        currentUser = defaultUser;
                        opcion = readOption(menuFinal, 2);
                        repeatAll = opcion == 1 ? true : false;
                        break;
                    }
                }

                //Se pregunta al usuario si desea realizar alguna accion mas en su sesion
                if (repeatSession)
                {
                    opcion = readOption(menuFinal, 2);
                    if(opcion == true){
                        if(opRegistradas == NUMTHREADS){
                            cout << "Error: Ha alcanzado la cantidad maxima de operaciones permitidas por sesion.";
                            repeatSession = false;
                        }else
                            repeatSession = true;

                    }else
                        repeatSession = false;
                    repeatSession = opcion == 1 ? true : false;
                }
            }

            cout << "Ejecutando todas las acciones indicadas..." << endl;
            sleep(3);
            for(int i=0; i<opRegistradas; i++){
                operation action = operationList[i];
                long rc;
                if(action.type == 100){
                    rc = pthread_create(&hilos[i], NULL, retiro, (void *)&action);
                    sleep(1);
                }
                else if(action.type == 200){
                    rc = pthread_create(&hilos[i], NULL, pago, (void *)&action);
                    sleep(1);
                }
                else if(action.type == 300){
                    rc = pthread_create(&hilos[i], NULL, prestamo, (void *)&action);
                    sleep(1);
                }
                else if(action.type == 400){
                    rc = pthread_create(&hilos[i], NULL, transf, (void *)&action);
                    sleep(1);
                }
            }

            for(int i= opRegistradas-1; i>=0;i--){
                pthread_join(hilos[i], NULL);
            }
            printf("Sesion terminada exitosamente con codigo: %d", generateNumber(6));
            //Se pregunta al usuario si desea realizar alguna accion mas en el cajero
            opcion = readOption(menuFinal, 2);
            repeatAll = opcion == 1 ? true : false;
        }
        else
        {
            //Se pregunta al usuario si desea realizar alguna accion mas en el cajero
            opcion = readOption(menuFinal, 2);
            repeatAll = opcion == 1 ? true : false;
        }
    }

    return 0;
}
