#ifndef T3ATTENDENCEDB_HPP
#define T3ATTENDENCEDB_HPP
/**
  ******************************************************************************
  * 公司    T3
  * 文件名   T3AttendenceDB.hpp
  * 作者    HXP
  * 版本    V1.0.0
  * 日期    2018.04.23
  * 说明    考勤表的操作
  ******************************************************************************
*/

#include "T3Database.hpp"
#include "../model/T3AttendenceModel.hpp"
namespace interface
{
class T3AttendenceDB
{
public:
  explicit T3AttendenceDB();
  /**
   * @brief addNewAttendence  添加一条考勤记录
   * @param attendence        [in] 考勤信息
   * @return                  返回值为正值时表示添加的记录的Id,为负值时表示添加失败
   */
  int addNewAttendence(model::T3AttendenceModel& attendence);
  /**
   * @brief selectSignIn  查询某人今天是否签到
   * @param userInfo      需要查询的人的信息
   * @return              true:已签到,false:未签到
   */
  bool selectSignIn(model::T3UserInfo userInfo);

};
}


#endif // T3ATTENDENCEDB_HPP