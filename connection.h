#ifndef CONNECTION_H
#define CONNECTION_H

#include <QtSql>
#include <QDebug>
#include <QFile>
#include <QMessageBox>

class Connection {
private:
    Connection(){}

public:
static QSqlDatabase& getInstance(bool isActive){
    static QSqlDatabase * db = nullptr;

    if(db == nullptr){
        db = new QSqlDatabase();
        *db = QSqlDatabase::addDatabase("QMYSQL","parserDota");
        QByteArray database;
        QByteArray username;
        QByteArray password;
        QFile file("db.ini");
        if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
            QMessageBox::warning(0, "falha geral", "problema em carregar arquivo db.ini");
        else{
            database = file.readLine();
            username = file.readLine();
            password = file.readLine();
        }
        database.resize(database.size()-1);
        username.resize(username.size()-1);
        password.resize(password.size()-1);

        db->setHostName("127.0.0.1");
        db->setDatabaseName(QString::fromUtf8(database));
        db->setUserName(QString::fromUtf8(username));
        db->setPassword(QString::fromUtf8(password));

        //qDebug() << "Conexao não existente" << db->userName() << db->password() << db->databaseName();
    }

    if(isActive && !db->isOpen()){
        if(db->open()){
           // qDebug() << "Conectado utilizando a connexao :" + db->databaseName();
        }
        else
            qDebug() << "Falha ao conectar: " << db->lastError().text();
    }
    else if(!isActive){
        db->close();
        //qDebug() << "Desconectado do banco de dados com sucesso!";
    }
    else if(db->isOpenError())
        qDebug() << "Desconectado do servidor, error :" + db->lastError().text() << endl << db->drivers();

    return *db;
}

static QSqlQuery& getQueryInstance(){
    static QSqlQuery * qry = nullptr;

    if(qry == nullptr)
        qry = new QSqlQuery(getInstance(true));
    else
        getInstance(true);

    return *qry;
}


static void destructor(){
    delete &getInstance(false);
    qDebug() << "Getinstance deletada";
    delete &getQueryInstance();
    qDebug() << "querymanager deletada";
}

};

#endif // CONNECTION_H
