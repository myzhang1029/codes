:: -*- coding: GBK -*-
@echo off
rem Automatically escape from the teacher's control.
rem Copyright (C) 2018-2019  Zhang Maiyun.
rem
rem This program is free software: you can redistribute it and/or modify
rem it under the terms of the GNU General Public License as published by
rem the Free Software Foundation, either version 3 of the License, or
rem (at your option) any later version.
rem
rem This program is distributed in the hope that it will be useful,
rem but WITHOUT ANY WARRANTY; without even the implied warranty of
rem MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
rem GNU General Public License for more details.
rem
rem You should have received a copy of the GNU General Public License
rem along with this program.  If not, see <https://www.gnu.org/licenses/>.
echo �ѿ�ħ���� ver 20190628
echo רҵ����StudentMain.exe����
echo �Զ���Ӧ�������
echo ����ɺ���
echo �ѿ�ħ����  Copyright (C) 2018-2019 Zhang Maiyun.
echo This program comes with ABSOLUTELY NO WARRANTY; for details,
echo visit "<https://www.gnu.org/licenses/>"
echo This is free software, and you are welcome to redistribute it
echo under certain conditions.

taskkill /F /IM StudentMain.exe 2>NUL
cdb -c q -pn StudentMain.exe >NUL 2>NUL

echo ��ɣ������������
pause >NUL