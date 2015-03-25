#ifndef CURRENCY_H
#define CURRENCY_H

#include <QHash>
#include <QString>
#include <QStringList>

class Currency {
public:
    static Currency *get(const QString name);
    QString name() const;

    bool hasRate(const QString currency) const;
    double to(const QString currency);

    static void addRate(const QString from, const QString to, double rate);
    static void addRate(const QString from, const QString to, const QString rate);
    void insert(const QString name, double conversion);
    static void printMap();
    static void fillInTable();
    static QStringList currencies();

private:
    QString _name;
    QHash<QString, double> rates;
    static QHash<QString, Currency *> map;

    Currency(const QString name) : _name(name) { rates[name] = 1.0; }
    Currency(const Currency &) {}

    void fillInCurrency(QStringList currencies, bool tryReverse = false);
    void findRate(const QString currency);
};

#endif // CURRENCY_H
