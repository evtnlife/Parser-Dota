#include "parser.h"
#include "ui_parser.h"
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>
#include <QNetworkReply>

Parser::Parser(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Parser)
{
    ui->setupUi(this);

    connect(ui->btn_steam_parser,SIGNAL(clicked(bool)),this,SLOT(getLeagues_id()));
    connect(this,SIGNAL(getNextMatch()),this,SLOT(trackDotaRequest()));
    connect(ui->btn_league_get,SIGNAL(clicked(bool)),this,SLOT(getLeagues_id()));

}

Parser::~Parser()
{
    delete ui;
}

void Parser::steamRequestFromMatchID()
{

    nan = new QNetworkAccessManager(this);
    QString key = ui->edit_key->text();

    if(min_match_id > 0 && league_id > 0){
        QString urlSteam = "https://api.steampowered.com/IDOTA2Match_570/GetMatchHistory/v1/?key="+key+"&league_id="+QString::number(league_id,'.',0)+"&start_at_match_id="+QString::number(min_match_id,'.',0)+"";
        QUrl url(urlSteam);
        qDebug() << url.url() << urlSteam;
        connect(nan,SIGNAL(finished(QNetworkReply*)),this,SLOT(steamParser(QNetworkReply*)));
        nan->get(QNetworkRequest(url));
    }
    else
        steamRequest();
}

void Parser::steamParser(QNetworkReply * reply)
{
    if(reply->error() == QNetworkReply::NoError){

        QSqlQuery querySelect = Connection::getQueryInstance();
        steamInformations info;
        QString logs;
        int contador = 0;

        connect(reply,SIGNAL(finished()),this,SLOT(deleteLater()));
        QByteArray jsonData = reply->readAll();
        reply->deleteLater();

        QJsonDocument document = QJsonDocument::fromJson(jsonData);
        QJsonObject object = document.object();

        QJsonValue value = object.value("result");
        value = value.toObject().value("matches");
        QJsonArray array = value.toArray();

        if(array.size() > 0){
            querySelect.prepare("SELECT match_id FROM partida WHERE match_id=:match_id");

            foreach (const QJsonValue & v, array) {
                if(v.toObject().value("radiant_team_id").toInt() != 0){
                    //test if match is already in db

                    querySelect.bindValue(":match_id", v.toObject().value("match_id").toDouble());// select match id from db
                    querySelect.exec();
                    if(!querySelect.next()){
                        ui->editSteamLogs->append("Match id: "+QString::number(v.toObject().value("match_id").toDouble(),'.',0)+" Series id: "+QString::number(v.toObject().value("series_id").toInt())+" Contador: "+QString::number(contador)+" datete time "+QDateTime::fromTime_t(v.toObject().value("start_time").toInt()).toString());

                        info.league_id = league_id;
                        info.series_id = v.toObject().value("series_id").toInt();
                        info.match_id = v.toObject().value("match_id").toDouble();
                        info.match_seq_num = v.toObject().value("match_seq_num").toDouble();
                        info.start_time = v.toObject().value("start_time").toInt();
                        info.radiant_team_id = v.toObject().value("radiant_team_id").toInt();
                        info.dire_team_id = v.toObject().value("dire_team_id").toInt();

                        if(existSteamInfo(info.match_id))
                            steamInfo.push_back(info);
                    }
                    else {
                        ui->editSteamLogs->append("Match id "+QString::number(v.toObject().value("match_id").toDouble(),'.',0)+" ja existente no banco de dados!");
                        qDebug() << "Steam parser ja adicionado no db " << v.toObject().value("match_id").toDouble() << league_id;
                    }

                }
                if(min_match_id > v.toObject().value("match_id").toDouble()){
                    min_match_id = v.toObject().value("match_id").toDouble();
                }

                else if(v.toObject().value("match_id").toDouble() > max_match_id){
                    max_match_id = v.toObject().value("match_id").toDouble();
                    ui->edit_maior_match_id->setValue(max_match_id);
                }
                contador++;
            }

            logs.append("Quantidade de Matchs salvos: "+QString::number(steamInfo.size()));
            ui->editSteamLogs->append(logs);

            if(contador > 1)//continua busca pelo menor match id até o resultado ser 1 ou menor que 1
                steamRequestFromMatchID();
            else {
                steamDataBase();
            }
        }
        else{
            reply->deleteLater();
            steamRequest();
        }
    }
}

void Parser::getLeagues_id()
{
    vectorLeagues_id.clear();
    QFile file(QFileDialog::getOpenFileName(this,"Atenção selecione o arquivo com os leagues id",""));
    if(file.open(QIODevice::ReadOnly | QFile::Text)){
        while(!file.atEnd()){
            QString value = file.readLine();

            vectorLeagues_id.push_back(value.toDouble());
            qDebug() << value;
        }
    }
    query = Connection::getQueryInstance();

    query.exec("START TRANSACITON");
    query.prepare("INSERT INTO leagues_id (league_id, isTracked) VALUES (:leagues_id, :isTracked)");
    QString logs;
    for(int i = 0; i<vectorLeagues_id.size();i++){
        // this is to test if exist in db if don't exist do it!
        QSqlQuery querySelect = Connection::getQueryInstance();
        querySelect.prepare("SELECT league_id FROM leagues_id WHERE league_id=:league_id");
        querySelect.bindValue(":league_id", vectorLeagues_id[i]);
        querySelect.exec();
        if(!querySelect.next()){
            query.bindValue(":leagues_id", vectorLeagues_id[i]);
            query.bindValue(":isTracked", 0);
            if(!query.exec()){
                qDebug() << query.lastError().text();
                query.exec("ROLLBACK");
                Connection::getInstance(false);
                break;
            }
            logs = "League id "+QString::number(vectorLeagues_id[i],'.',0)+" adicionado com sucesso ao banco de dados!";
            ui->editLeagueLogs->append(logs);
        }
        else{
            logs = "League id "+ QString::number(vectorLeagues_id[i],'.',0) +" ja existente no banco de dados não adicionado! ";
            ui->editLeagueLogs->append(logs);
        }
    }
    query.exec("COMMIT");

    vectorLeagues_id.clear();
    //get all leagues in db;
    query.exec("SELECT league_id FROM leagues_id");
    while(query.next()){
        double value = query.value(0).toDouble();
        vectorLeagues_id.push_back(value);
        qDebug()<<value;
    }
    Connection::getInstance(false);
    steamRequest();
}

void Parser::getMatch_id()
{
    query = Connection::getQueryInstance();
    vectorMatch_id.clear();
    query.prepare("SELECT isTracked FROM partida WHERE isTracked=0 and league_id=:league_id");
    query.bindValue(":match_id", match_id);
    query.exec();
    double val;
    while(query.next()){
        vectorMatch_id.push_back(val);
    }
}

bool Parser::existSteamInfo(double var)
{
    for(int i = 0; i < steamInfo.size(); i++){
        if(steamInfo[i].match_id == var)
            return false;
    }
    return true;
}

void Parser::steamDataBase()
{
    query = Connection::getQueryInstance();

    query.exec("START TRANSACTION");
    query.prepare("INSERT INTO partida (league_id, match_id, radiant_team_id, dire_team_id, start_time, series_id, match_seq_num, isTracked)"
                  " VALUES(:league_id,:match_id, :radiant_team_id, :dire_team_id, :start_time, :series_id, :match_seq_num, :isTracked)");

    for(int i =0; i < steamInfo.size(); i++){
        query.bindValue(":league_id", steamInfo[i].league_id);
        query.bindValue(":match_id", steamInfo[i].match_id);
        query.bindValue(":radiant_team_id", steamInfo[i].radiant_team_id);
        query.bindValue(":dire_team_id", steamInfo[i].dire_team_id);
        query.bindValue(":start_time", steamInfo[i].start_time);
        query.bindValue(":series_id", steamInfo[i].series_id);
        query.bindValue(":match_seq_num", steamInfo[i].match_seq_num);
        query.bindValue(":isTracked", false);

        qDebug() << "Passei no steam data base" << steamInfo[i].match_id;
        if(!query.exec()){
            qDebug() << query.lastError().text() << query.lastQuery();
            query.exec("ROLLBACK");
            Connection::getInstance(false);
            break;
        }
    }

    if(query.exec("COMMIT")){
        QString logs = "Inclusão no banco de ados concluida! Adicionado "+QString::number(steamInfo.size())+" registros no banco de dados";
        ui->editSteamLogs->append(logs);
        max_match_id = 0;
        min_match_id = 99999999999;
        steamInfo.clear();
        Connection::getInstance(false);
        trackDotaRequest();
    }
    Connection::getInstance(false);
}

void Parser::gameDataBase()
{
    query = Connection::getQueryInstance();

    query.exec("START TRANSACTION");

    //dire transaction!
    query.prepare("INSERT INTO equipes (match_id, team, player_slot, denies, last_hit, death, kills, assist, hero_id, account_id, level) "
                  "VALUES(:match_id, :team, :player_slot, :denies, :last_hit, :death, :kills, :assist, :hero_id, :account_id, :level)");
    for(int i =0; i<direPlayers.size(); i++){
        query.bindValue(":match_id", match_id);
        query.bindValue(":team", 0);
        query.bindValue(":player_slot", direPlayers[i].player_slot);
        query.bindValue(":denies", direPlayers[i].denies);
        query.bindValue(":last_hit", direPlayers[i].last_hit);
        query.bindValue(":kills", direPlayers[i].kills);
        query.bindValue(":death", direPlayers[i].death);
        query.bindValue(":assist", direPlayers[i].assist);
        query.bindValue(":hero_id", direPlayers[i].hero_id);
        query.bindValue(":account_id", direPlayers[i].account_id);
        query.bindValue(":level", direPlayers[i].level);

        if(!query.exec()){
            qDebug() << query.lastError().text();
            query.exec("ROLLBACK");
            Connection::getInstance(false);
            break;
        }
    }

    //radiant transaction!
    query.prepare("INSERT INTO equipes (match_id, team, player_slot, denies, last_hit, death, kills, assist, hero_id, account_id, level) "
                  "VALUES(:match_id, :team, :player_slot, :denies, :last_hit, :death, :kills, :assist, :hero_id, :account_id, :level)");
    for(int i =0; i<radiantPlayers.size(); i++){
        query.bindValue(":match_id", match_id);
        query.bindValue(":team", 1);
        query.bindValue(":player_slot", radiantPlayers[i].player_slot);
        query.bindValue(":denies", radiantPlayers[i].denies);
        query.bindValue(":last_hit", radiantPlayers[i].last_hit);
        query.bindValue(":kills", radiantPlayers[i].kills);
        query.bindValue(":death", radiantPlayers[i].death);
        query.bindValue(":assist", radiantPlayers[i].assist);
        query.bindValue(":hero_id", radiantPlayers[i].hero_id);
        query.bindValue(":account_id", radiantPlayers[i].account_id);
        query.bindValue(":level", radiantPlayers[i].level);

        if(!query.exec()){
            qDebug() << query.lastError().text();
            query.exec("ROLLBACK");
            Connection::getInstance(false);
            break;
        }
    }

    //actions ingame, kills ,deaths etc...
    query.prepare("INSERT INTO history_logs (match_id, action, timestamp, account_id, delta) "
                  "VALUES(:match_id, :action, :timestamp, :account_id, :delta)");
    for(int i =0; i<actionsRecord.size(); i++){
        query.bindValue(":match_id", match_id);
        query.bindValue(":action", actionsRecord[i].action);
        query.bindValue(":timestamp", actionsRecord[i].time_stamp);
        query.bindValue(":account_id", actionsRecord[i].account_id);
        query.bindValue(":delta", actionsRecord[i].delta);

        if(!query.exec()){
            qDebug() << query.lastError().text();
            query.exec("ROLLBACK");
            Connection::getInstance(false);
            break;
        }
    }

    //set tracked in db
    query.prepare("UPDATE partida set isTracked=1 WHERE match_id=:match_id");
    query.bindValue(":match_id", match_id);

    if(!query.exec()){
        qDebug() << query.lastError().text();
        query.exec("ROLLBACK");
        Connection::getInstance(false);
    }
    else {
        query.exec("COMMIT");
        ui->editGamesLogs->append("TrackDota requisição para o MatchID "+QString::number(match_id,'.',0)+" Concluida com sucesso!");
        Connection::getInstance(false);
        emit getNextMatch();
    }
}

void Parser::gameParser(QNetworkReply * reply)
{
    connect(reply,SIGNAL(finished()),this,SLOT(deleteLater()));
    QByteArray jsonData = reply->readAll();

    QJsonDocument document = QJsonDocument::fromJson(jsonData);
    QJsonObject object = document.object();


    QJsonValue value = object.value("dire");
    value = value.toObject().value("players");
    QJsonArray array = value.toArray();
    if(array.size() != 0){

        Dire dire;
        foreach (const QJsonValue & v, array){
            dire.player_slot = v.toObject().value("player_slot").toInt();
            dire.denies = v.toObject().value("denies").toInt();
            dire.last_hit = v.toObject().value("last_hits").toInt();
            dire.death = v.toObject().value("death").toInt();
            dire.kills = v.toObject().value("kills").toInt();
            dire.hero_id = v.toObject().value("hero_id").toInt();

            dire.account_id = v.toObject().value("account_id").toInt();
            dire.level = v.toObject().value("level").toInt();
            dire.assist = v.toObject().value("assist").toInt();

            direPlayers.push_back(dire);
        }

        value = object.value("radiant");
        value = value.toObject().value("players");

        array = value.toArray();

        Radiant radiant;
        foreach (const QJsonValue & v, array){
            radiant.player_slot = v.toObject().value("player_slot").toInt();
            radiant.denies = v.toObject().value("denies").toInt();
            radiant.last_hit = v.toObject().value("last_hits").toInt();
            radiant.death = v.toObject().value("death").toInt();
            radiant.kills = v.toObject().value("kills").toInt();
            radiant.hero_id = v.toObject().value("hero_id").toInt();

            radiant.account_id = v.toObject().value("account_id").toInt();
            radiant.level = v.toObject().value("level").toInt();
            radiant.assist = v.toObject().value("assist").toInt();
            radiantPlayers.push_back(radiant);
        }

        value = object.value("log");

        array = value.toArray();

        killRecord rec;

        foreach ( const QJsonValue& v, array) {
            rec.action = v.toObject().value("action").toString();
            rec.account_id = v.toObject().value("account_id").toInt();
            rec.delta = v.toObject().value("delta").toInt();
            rec.time_stamp = v.toObject().value("timestamp").toInt();

            if(rec.action == "kill" or rec.action == "death")
                actionsRecord.push_back(rec);
        }

        generalInfo info;

        info.finished = object.value("finished").toBool();
        info.updated = object.value("updated").toInt();
        info.winner = object.value("winner").toInt();

        infosGeral.push_back(info);

        qDebug() << "Game Parser" << match_id << league_id;
        gameDataBase();
    }
    else {
        steamRequest();
    }
}


void Parser::steamRequest()
{
    nan = new QNetworkAccessManager(this);
    ui->editGamesLogs->clear();
    QString key = ui->edit_key->text();

    league_id = 0;

    if(!vectorLeagues_id.isEmpty()){
        league_id = vectorLeagues_id[0];
        vectorLeagues_id.pop_front();
    }
    qDebug() << "FROM STEAM REQUEST LEAGUE ID: " << league_id << " Vector size" << vectorLeagues_id.size();

    if(league_id >= 1){
        QString urlSteam = "https://api.steampowered.com/IDOTA2Match_570/GetMatchHistory/v1/?key="+key+"&league_id="+QString::number(league_id,'.',0)+"";
        QUrl url(urlSteam);

        qDebug() << url.url() << urlSteam;
        connect(nan,SIGNAL(finished(QNetworkReply*)),this,SLOT(steamParser(QNetworkReply*)));
        nan->get(QNetworkRequest(url));
    }
}

void Parser::trackDotaRequest()
{
    nan = new QNetworkAccessManager(this);
    direPlayers.clear();
    radiantPlayers.clear();
    actionsRecord.clear();
    infosGeral.clear();

    match_id = 0;

    query = Connection::getQueryInstance();

    query.prepare("SELECT match_id FROM partida WHERE isTracked=0 and league_id=:league_id");
    query.bindValue(":league_id", league_id);
    query.exec();
    if(!query.next()){
        steamRequest();
    }else{
        match_id = query.value(0).toDouble();
    }
    Connection::getInstance(false);

    if(match_id >= 1){
        QString urlSteam;
        urlSteam = "http://www.trackdota.com/data/game/"+QString::number(match_id,'.',0)+"/live.json";
        QUrl url(urlSteam);

        ui->editGamesLogs->append("TrackDota Match ID" +QString::number(match_id,'.',0)+" está sendo carregado!");
        connect(nan,SIGNAL(finished(QNetworkReply*)),this,SLOT(gameParser(QNetworkReply*)));

        nan->get(QNetworkRequest(url));
    }
}

