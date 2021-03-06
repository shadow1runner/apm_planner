/*===================================================================
APM_PLANNER Open Source Ground Control Station

(c) 2015 APM_PLANNER PROJECT <http://www.diydrones.com>

This file is part of the APM_PLANNER project

    APM_PLANNER is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    APM_PLANNER is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with APM_PLANNER. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/
/**
 * @file
 *   @brief AP2DataPlot internal data model
 *
 *   @author Michael Carpenter <malcom2073@gmail.com>
 */


#include "AP2DataPlot2DModel.h"
#include <QSqlQuery>
#include <QDebug>
#include <QSqlRecord>
#include <QSqlField>
#include <QSqlError>
#include <QUuid>
#include <QsLog.h>

/*
 * This model holds everything in memory in a sqlite database.
 * There are two system tables, then unlimited number of message tables.
 *
 * System Tables:
 *
 * FMT (idx integer PRIMARY KEY, typeid integer,length integer,name varchar(200),format varchar(6000),val varchar(6000));
 *  idx: Index - Position in the file in relation to other values
 *  typeid: typeid, this is unused in most cases
 *  length: length, this is unused in most cases
 *  name: Name of the type defined by the format: ATUN
 *  format: format of the message: BBfffff
 *  val: Comma deliminted field with names of the message fields: Axis,TuneStep,RateMin,RateMax,RPGain,RDGain,SPGain
 *
 * INDEX (idx integer PRIMARY KEY, value varchar(200));
 *  idx: Index: Index on graph/table
 *  value: The table name (message type name) that the index points to
 *
 * Message tables are formatted as such:
 *
 * ATUN (idx integer PRIMARY KEY, Axis integer, TuneStep integer, RateMin real, RateMax real, RPGain real, RDGain real, SPGain real);
 *  The types are defined by the format (in this case, BBfffff)
 *  inside AP2DataPlot2DModel::makeCreateTableString.
 */
AP2DataPlot2DModel::AP2DataPlot2DModel(QObject *parent) :
    QAbstractTableModel(parent),
    m_databaseName(QUuid::createUuid().toString()),
    m_rowCount(0),
    m_columnCount(0),
    m_currentRow(0),
    m_fmtIndex(0),
    m_firstIndex(0),
    m_lastIndex(0)
{
    m_sharedDb = QSqlDatabase::addDatabase("QSQLITE",m_databaseName);
    m_sharedDb.setDatabaseName(":memory:");

    //  Open DB and start transaction
    if (!m_sharedDb.open())
    {
        setError("Error opening shared database " + m_sharedDb.lastError().text());
        return;
    }
    if (!m_sharedDb.transaction())
    {
        setError("Unable to start database transaction "  + m_sharedDb.lastError().text());
        return;
    }

    // Create FMT Table and prepare insert query
    if (!createFMTTable())
    {
        return; //Error already emitted.
    }
    m_fmtInsertQuery = queryPtr(new QSqlQuery(m_sharedDb));
    if (!createFMTInsert(m_fmtInsertQuery))
    {
        return;  //Error already emitted.
    }

    // Create index table and prepare insert query
    if (!createIndexTable())
    {
        return;   //Error already emitted.
    }
    m_indexinsertquery = queryPtr(new QSqlQuery(m_sharedDb));
    if (!createIndexInsert(m_indexinsertquery))
    {
        return;  //Error already emitted.
    }

    if (!m_sharedDb.commit())
    {
        setError("Unable to commit database transaction "  + m_sharedDb.lastError().text());
        return;
    }
}

AP2DataPlot2DModel::~AP2DataPlot2DModel()
{
    QSqlDatabase::removeDatabase(m_databaseName);
}

QMap<QString,QList<QString> > AP2DataPlot2DModel::getFmtValues()
{
    QMap<QString,QList<QString> > retval;
    QSqlQuery fmtquery(m_sharedDb);
    fmtquery.prepare("SELECT * FROM 'FMT';");
    fmtquery.exec();
    while (fmtquery.next())
    {
        QSqlRecord record = fmtquery.record();
        QString name = record.value(3).toString();
        QSqlQuery indexquery(m_sharedDb);
        if (!indexquery.prepare("SELECT * FROM 'INDEX' WHERE value = '" + name + "';"))
        {
            continue;
        }
        if (!indexquery.exec())
        {
            continue;
        }
        if (!indexquery.next())
        {
            //No records
            continue;
        }
        if (!m_headerStringList.contains(name))
        {
            continue;
        }
        retval.insert(name,QList<QString>());
        for (int i=0;i<m_headerStringList[name].size();i++)
        {
            retval[name].append(m_headerStringList[name].at(i));
        }
    }
    return retval;
}
QString AP2DataPlot2DModel::getFmtLine(const QString& name)
{
    QSqlQuery fmtquery(m_sharedDb);
    fmtquery.prepare("SELECT * FROM 'FMT' WHERE name = '" + name + "';");
    if (!fmtquery.exec())
    {
        //QMessageBox::information(0,"Error","Error selecting from table 'FMT' " + m_sharedDb.lastError().text());
        //return;
    }
    if (fmtquery.next())
    {
        QSqlRecord record = fmtquery.record();
        QString name = record.value(3).toString();
        QString vars = record.value(5).toString();
        QString format = record.value(4).toString();
        int size = 0;
        for (int i=0;i<format.size();i++)
        {
            if (format.at(i).toLatin1() == 'n')
            {
                size += 4;
            }
            else if (format.at(i).toLatin1() == 'N')
            {
                size += 16;
            }
            else if (format.at(i).toLatin1() == 'Z')
            {
                size += 64;
            }
            else if (format.at(i).toLatin1() == 'f')
            {
                size += 4;
            }
            else if (format.at(i).toLatin1() == 'Q')
            {
                size += 8;
            }
            else if (format.at(i).toLatin1() == 'q')
            {
                size += 8;
            }
            else if ((format.at(i).toLatin1() == 'i') || (format.at(i).toLatin1() == 'I') || (format.at(i).toLatin1() == 'e') || (format.at(i).toLatin1() == 'E')  || (format.at(i).toLatin1() == 'L'))
            {
                size += 4;
            }
            else if ((format.at(i).toLatin1() == 'h') || (format.at(i).toLatin1() == 'H') || (format.at(i).toLatin1() == 'c') || (format.at(i).toLatin1() == 'C'))
            {
                size += 2;
            }
            else if ((format.at(i).toLatin1() == 'b') || (format.at(i).toLatin1() == 'B') || (format.at(i).toLatin1() == 'M'))
            {
                size += 1;
            } else {
                QLOG_DEBUG() << "Unknown format character (" << format.at(i).toLatin1() << "); export will be bad";
            }
        }
        QString formatline = "FMT, " + QString::number(record.value(1).toInt()) + ", " + QString::number(size+3) + ", " + name + ", " + format + ", " + vars;
        return formatline;
    }
    return "";
}
QMap<quint64,QString> AP2DataPlot2DModel::getModeValues()
{
    QMap<quint64,QString> retval;
    QSqlQuery modequery(m_sharedDb);
    modequery.prepare("SELECT * FROM 'MODE';");
    if (!modequery.exec())
    {
        //No mode?
        QLOG_DEBUG() << "Graph loaded with no mode table. Running anyway, but text modes will not be available";
        modequery.prepare("SELECT * FROM 'HEARTBEAT';");
        if (!modequery.exec())
        {
            QLOG_DEBUG() << "Graph loaded with no heartbeat either. No modes available";
        }
    }
    QString lastmode = "";
    MAV_TYPE foundtype = MAV_TYPE_GENERIC;
    bool custom_mode = false;

    while (modequery.next())
    {
        QSqlRecord record = modequery.record();
        quint64 index = record.value(0).toLongLong();
        QString mode = "";
        if (record.contains("Mode"))
        {
            mode = record.value("Mode").toString();
        }
        else if (record.contains("custom_mode"))
        {
            custom_mode = true;
            int modeint = record.value("custom_mode").toString().toInt();
            if (foundtype == MAV_TYPE_GENERIC)
            {
                int type = record.value("type").toString().toInt();
                foundtype = static_cast<MAV_TYPE>(type);
            }
            if (foundtype == MAV_TYPE_FIXED_WING)
            {
                mode = ApmPlane::stringForMode(modeint);
            }
            else if (foundtype == MAV_TYPE_QUADROTOR || foundtype == MAV_TYPE_COAXIAL || foundtype == MAV_TYPE_HELICOPTER || \
                     foundtype == MAV_TYPE_HEXAROTOR || foundtype == MAV_TYPE_OCTOROTOR || foundtype == MAV_TYPE_TRICOPTER)
            {
                mode = ApmCopter::stringForMode(modeint);
            }
            else if (foundtype == MAV_TYPE_GROUND_ROVER)
            {
                mode = ApmRover::stringForMode(modeint);
            }
            else
            {
                mode = QString::number(static_cast<int>(modeint));
            }
        }
        bool ok = false;

        if (!ok && !custom_mode)
        {
            if (record.contains("ModeNum"))
            {
                mode = record.value("ModeNum").toString();
            }
            else
            {
                QLOG_DEBUG() << "Unable to determine Mode number in log" << record.value("Mode").toString();
                mode = record.value("Mode").toString();
            }
        }
        if (lastmode != mode)
        {
            retval.insert(index,mode);
            lastmode = mode;
        }

    }
    return retval;
}


QMap<quint64,ErrorType> AP2DataPlot2DModel::getErrorValues()
{
    QMap<quint64,ErrorType> retval;
    QSqlQuery errorquery(m_sharedDb);
    errorquery.prepare("SELECT * FROM 'ERR';");
    if (errorquery.exec())
    {
        while (errorquery.next())
        {
            QSqlRecord record = errorquery.record();
            quint64 index = static_cast<quint64>(record.value(0).toLongLong());
            ErrorType error;

            if (!error.setFromSqlRecord(record))
            {
                QLOG_DEBUG() << "Not all data could be read from SQL-Record. Schema mismatch?!";
            }

            retval.insert(index, error);
        }
    }
    else
    {
        //Errorquery returned no result - No error?
        QLOG_DEBUG() << "Graph loaded with no error table. This is perfect!";
    }

    return retval;
}


QVariant AP2DataPlot2DModel::headerData ( int section, Qt::Orientation orientation, int role) const
{
    if (section == -1)
    {
        return QVariant();
    }
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }
    if (orientation == Qt::Vertical)
    {
        return QVariant();
    }
    if (section == 0)
    {
        return "Index";
    }
    if (section == 1)
    {
        return "MSG Type";
    }
    if ((section-2) >= m_currentHeaderItems.size())
    {
        return QVariant("");
    }
    return m_currentHeaderItems.at(section-2);
}

int AP2DataPlot2DModel::rowCount( const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return m_rowCount;
}

int AP2DataPlot2DModel::columnCount ( const QModelIndex & parent) const
{
    Q_UNUSED(parent)
    return m_columnCount;
}

QVariant AP2DataPlot2DModel::data ( const QModelIndex & index, int role) const
{
    if ((role != Qt::DisplayRole) || !index.isValid())
    {
        return QVariant();
    }
    if (index.row() >= m_rowCount)
    {
        QLOG_ERROR() << "Accessing a Database row that does not exist! Row was: " << index.row();
        return QVariant();
    }

    if (index.column() == 0)
    {
        // Column 0 is the DB index of the log data
        return QVariant(QString::number(m_rowIndexToDBIndex[index.row()].first));
    }
    if (index.column() == 1)
    {
        // Column 1 is the name of the log data (ATT,ATUN...)
        return QVariant(m_rowIndexToDBIndex[index.row()].second);
    }

    // The data is mostly read row by row. This means first all columns of row 1 are read and then
    // all columns of row 2 and than all colums of row 3 and so on.
    // This cache reads a whole row from DB and caches it internally. All consecutive accesses to this
    // row will be answered from the cache.
    if (index.row() != m_prefetchedRowIndex.row())
    {
        m_prefetchedRowData.clear();
        QString tableIndex = QString::number(m_rowIndexToDBIndex[index.row()].first);
        QString tableName  = m_rowIndexToDBIndex[index.row()].second;

        queryPtr selectRowQuery = m_msgNameToPrepearedSelectQuery.value(tableName);
        selectRowQuery->bindValue(":val", tableIndex);
        if (selectRowQuery->exec())
        {
            int recordCount = selectRowQuery->record().count();
            if (!selectRowQuery->next())
            {
                return QVariant();
            }
            for (int i = 0; i < recordCount; ++i)
            {
                m_prefetchedRowData.push_back(selectRowQuery->value(i));
            }
            m_prefetchedRowIndex = index;
            selectRowQuery->finish();   // must be called because we will reuse this query
        }
        else
        {
            qDebug() << "Unable to exec table query:" << selectRowQuery->lastError().text();
        }
    }

    if ((index.column()-1) >= m_prefetchedRowData.size())
    {
        return QVariant();
    }
    return m_prefetchedRowData[index.column()-1];
}

void AP2DataPlot2DModel::selectedRowChanged(QModelIndex current,QModelIndex previous)
{
    Q_UNUSED(previous)
    if (!current.isValid())
    {
        return;
    }
    if (m_currentRow == current.row())
    {
        return;
    }
    m_currentRow = current.row();
    if (current.row() < m_fmtStringList.size())
    {
        m_currentHeaderItems = QList<QString>();
        emit headerDataChanged(Qt::Horizontal,0,9);
        return;
    }
    //Grab the index

    if (current.row() < m_rowCount)
    {
        m_currentHeaderItems = m_headerStringList.value(m_rowIndexToDBIndex[current.row()].second);
    }
    else
    {
        m_currentHeaderItems = QList<QString>();
    }
    emit headerDataChanged(Qt::Horizontal,0,9);
}

bool AP2DataPlot2DModel::hasType(const QString& name)
{
    return m_msgNameToPrepearedInsertQuery.contains(name);
}

bool AP2DataPlot2DModel::addType(QString name,int type,int length,QString types,QStringList names)
{
    if (!m_msgNameToPrepearedInsertQuery.contains(name))
    {
        QString createstring = makeCreateTableString(name,types,names);
        QString variablenames = "";
        for (int i=0;i<names.size();i++)
        {
            variablenames += names.at(i) + ((i < names.size()-1) ? "," : "");
        }
        QString insertstring = makeInsertTableString(name,names);

        //if (!query->prepare("INSERT INTO 'FMT' (idx,typeid,length,name,format,val) values (:idx,:typeid,:length,:name,:format,:val);"))
        m_fmtInsertQuery->bindValue(":idx",m_fmtIndex++);
        m_fmtInsertQuery->bindValue(":typeid",type);
        m_fmtInsertQuery->bindValue(":length",length);
        m_fmtInsertQuery->bindValue(":name",name);
        m_fmtInsertQuery->bindValue(":format",types);
        m_fmtInsertQuery->bindValue(":val",variablenames);
        QList<QString> list;
        list.append("FMT");
        list.append(QString::number(m_fmtIndex-1));
        list.append(QString::number(type));
        list.append(QString::number(length));
        list.append(types);
        list.append(variablenames);
        m_fmtStringList.append(list);
        if (!m_fmtInsertQuery->exec())
        {
            setError("FAILED TO FMT: " + m_fmtInsertQuery->lastError().text());
            return false;
        }
        m_indexinsertquery->bindValue(":idx",m_fmtIndex-1);
        m_indexinsertquery->bindValue(":value","FMT");
        if (!m_indexinsertquery->exec())
        {
            setError("Error execing index: " + name + " " + m_indexinsertquery->lastError().text());
            return false;
        }

        // Create table for measurement of type "name"
        QSqlQuery create(m_sharedDb);
        if (!create.prepare(createstring))
        {
            setError("Unable to create: " + create.lastError().text());
            return false;
        }
        if (!create.exec())
        {
            setError("Unable to exec create: " + create.lastError().text());
            return false;
        }
        // Prepare an insert query for the created table above and store it to speed up the inserting process later
        queryPtr prepQuery = queryPtr(new QSqlQuery(m_sharedDb));
        if (!prepQuery->prepare(insertstring.replace("insert or replace","insert")))
        {
            setError("Error preparing insertquery: " + name + " " + prepQuery->lastError().text());
            return false;
        }
        m_msgNameToPrepearedInsertQuery.insert(name, prepQuery);
        // And prepare a select query for rows of this table
        prepQuery = queryPtr(new QSqlQuery(m_sharedDb));
        if (!prepQuery->prepare("SELECT * FROM " + name + " WHERE idx = :val"))
        {
            setError("Error preparing select query: " + name + " " + prepQuery->lastError().text());
            return false;
        }
        m_msgNameToPrepearedSelectQuery.insert(name, prepQuery);

    }
    if (!m_headerStringList.contains(name))
    {
        m_headerStringList.insert(name,QList<QString>());
        foreach (QString val,names)
        {
            m_headerStringList[name].append(val);
        }
    }
    return true;
}
QMap<quint64,QVariant> AP2DataPlot2DModel::getValues(const QString& parent,const QString& child)
{
    int index = getChildIndex(parent,child);
    QSqlQuery itemquery(m_sharedDb);
    itemquery.prepare("SELECT * FROM '" + parent + "';");
    itemquery.exec();
    QMap<quint64,QVariant> retval;
    while (itemquery.next())
    {
        QSqlRecord record = itemquery.record();
        quint64 graphindex = record.value(0).toLongLong();
        QVariant graphvalue= record.value(index+1);
        retval.insert(graphindex,graphvalue);
    }
    return retval;
}

int AP2DataPlot2DModel::getChildIndex(const QString& parent,const QString& child)
{

    QSqlQuery tablequery(m_sharedDb);
    //tablequery.prepare("SELECT * FROM '" + parent + "';");
    if (!tablequery.prepare("SELECT * FROM 'FMT' WHERE name = '" + parent + "';"))
    {
        QLOG_DEBUG() << "Error preparing select:" + tablequery.lastError().text();
        return -1;
    }
    if (!tablequery.exec())
    {
        QLOG_DEBUG() << "Error execing select:" + tablequery.lastError().text();
        return -1;
    }
    if (!tablequery.next())
    {
        return -1;
    }
    QSqlRecord record = tablequery.record();
    QStringList valuessplit = record.value(5).toString().split(","); //comma delimited list of names
    bool found = false;
    int index = 0;
    for (int i=0;i<valuessplit.size();i++)
    {
        if (valuessplit.at(i) == child)
        {
            found = true;
            index = i;
            i = valuessplit.size();
        }
    }
    if (!found)
    {
        return -1;
    }
    return index;
}
bool AP2DataPlot2DModel::startTransaction()
{
    if (!m_sharedDb.transaction())
    {
        setError("Unable to start transaction to database: " + m_sharedDb.lastError().text());
        return false;
    }
    return true;
}
bool AP2DataPlot2DModel::endTransaction()
{
    if (!m_sharedDb.commit())
    {
        setError("Unable to commit to database: " + m_sharedDb.lastError().text());
        return false;
    }
    return true;
}

bool AP2DataPlot2DModel::addRow(QString name,QList<QPair<QString,QVariant> >  values,quint64 index)
{
    if (m_firstIndex == 0)
    {
        m_firstIndex = index;
    }
    m_lastIndex = index;

    //Add a row to a previously defined message type using the already prepared insert query
    queryPtr insertQuery = m_msgNameToPrepearedInsertQuery.value(name);
    if (!insertQuery)
    {
        setError("No prepared insert query available for message: " + name);
        return false;
    }
    insertQuery->bindValue(":idx", index);
    for (int i=0;i<values.size();i++)
    {
        insertQuery->bindValue(":" + values.at(i).first, values.at(i).second);
    }
    if (!insertQuery->exec())
    {
        setError("Error execing insert query: " + insertQuery->lastError().text());
        return false;
    }
    else
    {
        insertQuery->finish();  // must be called in case of a reusage of this query
        m_indexinsertquery->bindValue(":idx", index);
        m_indexinsertquery->bindValue(":value", name);
        if (!m_indexinsertquery->exec())
        {
            setError("Error execing:" + m_indexinsertquery->executedQuery() + " error was " + m_indexinsertquery->lastError().text());
            return false;
        }
    }

    // Our table model is larger than the number of columns we insert:
    // +1 for the index column (in column 0)
    // +1 for the table/message type (in column 1)
    if (values.size()+2 > m_columnCount)
    {
        m_columnCount = values.size() +2;
    }

    m_rowIndexToDBIndex.push_back(QPair<quint64,QString>(index,name));
    m_rowCount++;
    return true;
}
QString AP2DataPlot2DModel::makeCreateTableString(QString tablename, QString formatstr,QStringList variablestr)
{
    QString mktable = "CREATE TABLE '" + tablename + "' (idx integer PRIMARY KEY";
    for (int j=0;j<variablestr.size();j++)
    {
        QString name = variablestr[j].trimmed();
        //name = "\"" + name + "\"";
        QChar typeCode = formatstr.at(j);
        if (typeCode == 'b') //int8_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'B') //uint8_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'h') //int16_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'H') //uint16_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'i') //int32_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'I') //uint32_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'f') //float
        {
            mktable.append("," + name + " real");
        }
        else if (typeCode == 'd') //double
        {
            mktable.append("," + name + " real");
        }
        else if (typeCode == 'N') //char(16)
        {
            mktable.append("," + name + " text");
        }
        else if (typeCode == 'Z') //char(64)
        {
            mktable.append("," + name + " text");
        }
        else if (typeCode == 'c') //int16_t * 100
        {
            mktable.append("," + name + " real");
        }
        else if (typeCode == 'C') //uint16_t * 100
        {
            mktable.append("," + name + " real");
        }
        else if (typeCode == 'e') //int32_t * 100
        {
            mktable.append("," + name + " real");
        }
        else if (typeCode == 'E') //uint32_t * 100
        {
            mktable.append("," + name + " real");
        }
        else if (typeCode == 'L') //uint32_t lon/lat
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'M') //uint8_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'q') //int64_t
        {
            mktable.append("," + name + " integer");
        }
        else if (typeCode == 'Q') //uint64_t
        {
            mktable.append("," + name + " integer");
        }
        else
        {
            QLOG_DEBUG() << "AP2DataPlotThread::makeCreateTableString(): NEW UNKNOWN VALUE" << typeCode;
        }
    }
    mktable.append(");");
    return mktable;
}
QString AP2DataPlot2DModel::makeInsertTableString(QString tablename, QStringList variablestr)
{
    QString inserttable = "insert or replace into '" + tablename + "' (idx";
    QString insertvalues = "(:idx,";
    for (int j=0;j<variablestr.size();j++)
    {
        QString name = variablestr[j].trimmed();
        inserttable.append("," + name + "");
        insertvalues.append(":" + name + ((j < variablestr.size()-1) ? "," : ""));
    }
    inserttable.append(")");
    insertvalues.append(")");
    QString final = inserttable + " values " + insertvalues + ";";
    return final;
}
bool AP2DataPlot2DModel::createFMTTable()
{
    QSqlQuery fmttablecreate(m_sharedDb);
    if (!fmttablecreate.prepare("CREATE TABLE 'FMT' (idx integer PRIMARY KEY, typeid integer,length integer,name varchar(200),format varchar(6000),val varchar(6000));"))
    {
        setError("Prapre create FMT table failed: " + fmttablecreate.lastError().text());
        return false;
    }
    if (!fmttablecreate.exec())
    {
        QLOG_ERROR() << "Error creating FMT table: " + fmttablecreate.lastError().text();
        setError("Exec create FMT table failed: " + fmttablecreate.lastError().text());
        return false;
    }
    return true;
}
bool AP2DataPlot2DModel::createFMTInsert(queryPtr &query)
{
    if (!query->prepare("INSERT INTO 'FMT' (idx,typeid,length,name,format,val) values (:idx,:typeid,:length,:name,:format,:val);"))
    {
        setError("Insert into FMT prepare failed: " + query->lastError().text());
        return false;
    }
    return true;
}
bool AP2DataPlot2DModel::createIndexTable()
{
    QSqlQuery indextablecreate(m_sharedDb);
    if (!indextablecreate.prepare("CREATE TABLE 'INDEX' (idx integer PRIMARY KEY, value varchar(200));"))
    {
        setError("Error preparing INDEX table: " + m_sharedDb.lastError().text());
        return false;
    }
    if (!indextablecreate.exec())
    {
        setError("Error creating INDEX table: " + m_sharedDb.lastError().text());
        return false;
    }
    return true;
}
bool AP2DataPlot2DModel::createIndexInsert(queryPtr &query)
{
    if (!query->prepare("INSERT INTO 'INDEX' (idx,value) values (:idx,:value);"))
    {
        setError("Insert into Index prepare failed: " + query->lastError().text());
        return false;
    }
    return true;
}
void AP2DataPlot2DModel::setError(QString error)
{
    QLOG_ERROR() << error;
    m_error = error;
}
quint64 AP2DataPlot2DModel::getLastIndex()
{
    return m_lastIndex;
}

quint64 AP2DataPlot2DModel::getFirstIndex()
{
    return m_firstIndex;
}
