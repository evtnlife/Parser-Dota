#ifndef PARSER_H
#define PARSER_H

#include <QMainWindow>
#include "connection.h"
#include <QNetworkAccessManager>

struct Dire;
struct Radiant;
struct killRecord;
struct generalInfo;
struct steamInformations;

namespace Ui {
class Parser;
}

class Parser : public QMainWindow
{
    Q_OBJECT

public:
    explicit Parser(QWidget *parent = 0);
    ~Parser();

    QVector<Dire> direPlayers;
    QVector<Radiant> radiantPlayers;
    QVector<killRecord> actionsRecord;
    QVector<generalInfo> infosGeral;
    QVector<steamInformations> steamInfo;
    QVector<double> vectorLeagues_id;
    QVector<double> vectorMatch_id;

protected slots:
    void steamRequest();
    void trackDotaRequest();
    void gameParser(QNetworkReply * reply);
    void steamParser(QNetworkReply * reply);
    void getLeagues_id();
    void getMatch_id(); // this function get a next match_id

signals:
    void getNextMatch();

private:
    Ui::Parser *ui;
    QSqlQuery query;
    //private vars
    double max_match_id = 0;
    double min_match_id = 9999999999999;
    int contador_vector_steam = 0;
    double match_id;
    double match_id_anterior;
    double league_id;

    void steamRequestFromMatchID();
    bool existSteamInfo(double);
    void steamDataBase();
    void gameDataBase();

    QNetworkAccessManager * nan;
};


struct Dire{
    int player_slot;
    int denies;
    int last_hit;
    int death;
    int kills;
    int assist;
    int hero_id;
    int account_id;
    int level;
};

struct Radiant{
    int player_slot;
    int denies;
    int last_hit;
    int death;
    int kills;
    int assist;
    int hero_id;
    int account_id;
    int level;
};

struct killRecord{
    QString action;
    int account_id;
    int time_stamp;
    int delta;
};

struct generalInfo{
    int updated;
    bool finished;
    int winner;
};


struct steamInformations{
    double league_id;
    int series_id;
    double match_id;
    int match_seq_num;
    int start_time;
    int radiant_team_id;
    int dire_team_id;
};


#endif // PARSER_H
