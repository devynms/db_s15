from sys import argv
from collections import Counter
import re
import subprocess
import random
import sys
import os
sys.path.append('./sql/pyutil')
import sql_populate
from datetime import datetime


if __name__ == '__main__':
	pap_list = []
	pap_lit = eval(open("./test.txt", 'r').read())
	for pap_dict in pap_lit:
		pub = sql_populate.Journal('journal', 1, 1, sql_populate.Publisher('Publisher'), datetime.now().date().isoformat(), topics=['arbitrary', 'fake', 'ham'])
		paper = sql_populate.AcademicPaper(pap_dict['title'], pub, pap_dict['keywords'],
	 		pap_dict['topics'], pap_dict['author'], pap_dict['citations'], pap_dict['abstract'])
		pap_list.append(paper)
	sql_populate.publish_sql_structures(sql_populate._get_database_connection(), sql_populate.sql_structures_from_papers(pap_list, []))