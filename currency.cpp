#include "currency.h"

#include <QDebug>

//#define DEBUG_CURRENCY

QHash<QString, Currency *> Currency::map;

Currency *Currency::get(const QString name) {
    Currency *c = map.value(name, 0);
    if (!c) {
        c = new Currency(name);
        map[name] = c;
    }
    return c;
}

QString Currency::name() const {
    return _name;
}

bool Currency::hasRate(const QString currency) const
{
    return rates.contains(currency);
}

double Currency::to(const QString currency) {
    if (!rates.contains(currency))
        findRate(currency);
    if (rates.contains(currency))
        return rates[currency];
    qDebug() << "No known conversion from" << name() << "to" << currency;
    return 0.0;
}

void Currency::insert(const QString name, double conversion) {
    //qDebug() << qPrintable(QString("%1 => %2 : %3").arg(this->name(), name).arg(conversion));
    if (rates.contains(name))
        qDebug() << "WARNING: old value for" << name << "is" << rates[name];
    rates[name] = conversion;
    // For reverse mappings of currencies with no forward mappings
    if (!map.contains(name))
        get(name);
}

void Currency::addRate(const QString from, const QString to, double rate) {
    if (from.simplified().isEmpty() || to.simplified().isEmpty())
        return;
    Currency::get(from.simplified())->insert(to.simplified(), rate);
}

void Currency::addRate(const QString from, const QString to, const QString rate) {
    bool ok = false;
    double rateAsDouble = rate.simplified().toDouble(&ok);
    if (ok)
        addRate(from, to, rateAsDouble);
}

void Currency::printMap() {
    qDebug("====================");
    foreach (const QString &key, map.keys()) {
        qDebug("%s:", qPrintable(key));
        Currency *c = map[key];
        foreach (const QString &key2, c->rates.keys()) {
            if (key == key2)
                continue;
            qDebug("\t%s:  %18.16f", qPrintable(key2), c->to(key2));
        }
    }
}

void Currency::fillInTable()
{
    // Goal: fill out the rates map for each currency to have a "to" mapping
    //       to each of the other currencies
    //       - not concerned with efficiency here

    // Plan:
    //       1. Make a set of all currencies.
    //       2. For each member of the set, try to fill in missing rates for the
    //       other currencies. Do this by looking to see if any of them can be
    //       satisfied in 1 degree of separation. If not, skip it, and move on.
    //       3. Repeat step 2 until no more progress is made (either by success
    //       or failure). If no more progress is made and the table is still
    //       incomplete, try using reverse conversions.

    foreach (Currency *c, map.values())
        foreach (const QString &currency, map.keys())
            c->findRate(currency);
}

QStringList Currency::currencies() {
    return map.keys();
}

void Currency::fillInCurrency(QStringList currencies, bool tryReverse) {
#ifdef DEBUG_CURRENCY
    qDebug() << "    fillInCurrency:" << name() << "->" << currencies;
#endif
    // Make sure this currency has a rate for a direct conversion
    // to each of the currencies passed in.
    foreach (const QString &destination, currencies) {
        // If the mapping already exists, we're good.
        if (hasRate(destination))
            continue;

        // No mapping.
        // For each mapping that we do know, is there a mapping from
        // that currency to the one we want?
        foreach (const QString &key, rates.keys()) {
            Currency *c2 = Currency::get(key);
            if (c2->hasRate(destination)) {
                insert(destination, rates[key] * c2->rates[destination]);
#ifdef DEBUG_CURRENCY
                qDebug() << "        insert:" << name() << destination << rates[destination];
#endif
                break;
            }
        }

        if (hasRate(destination) || !tryReverse)
            continue;

        Currency *c3 = Currency::get(destination);
        // See if there's a direct reversal first
        if (c3->hasRate(name())) {
            insert(destination, 1.0 / c3->rates[name()]);
#ifdef DEBUG_CURRENCY
            qDebug() << "        insert:" << name() << destination << rates[destination];
#endif
            continue;
        }
    }
}

void Currency::findRate(const QString currency) {
#ifdef DEBUG_CURRENCY
    if (!hasRate(currency))
        qDebug() << "findrate:" << name() << "->" << currency;
#endif
    bool tryReverse = false;
    bool workPerformed = true;
    QSet<Currency *> others = map.values().toSet();
    others -= this;
    others -= get(currency);
    while (!hasRate(currency)) {
        // Only use reverse conversions if all possible forward conversions
        // have been exhausted and the table is still incomplete.
        tryReverse = !workPerformed;
        workPerformed = false;

        fillInCurrency(QStringList() << currency);
        if (hasRate(currency))
            break;

        // Check each currency for missing mappings
        foreach (Currency *c, others) {
            int size = c->rates.size();
            c->fillInCurrency(QStringList() << currency, tryReverse);
            workPerformed = workPerformed || (size != c->rates.size());
        }

        if (!workPerformed && tryReverse)
            break;
    }
}
