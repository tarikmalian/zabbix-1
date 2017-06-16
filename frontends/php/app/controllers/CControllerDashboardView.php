<?php
/*
** Zabbix
** Copyright (C) 2001-2017 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/


require_once dirname(__FILE__).'/../../include/blocks.inc.php';

class CControllerDashboardView extends CControllerDashboardAbstract {

	private $dashboard;

	protected function init() {
		$this->disableSIDValidation();
	}

	protected function checkInput() {
		$fields = [
			'fullscreen' =>			'in 0,1',
			'dashboardid' =>		'db dashboard.dashboardid',
			'source_dashboardid' =>	'db dashboard.dashboardid',
			'groupid' =>			'db groups.groupid',
			'hostid' =>				'db hosts.hostid',
			'new' =>				'in 1'
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$this->setResponse(new CControllerResponseFatal());
		}

		return $ret;
	}

	protected function checkPermissions() {
		if ($this->getUserType() < USER_TYPE_ZABBIX_USER) {
			return false;
		}

		$this->dashboard = $this->getDashboard();

		return !$this->hasInput('dashboardid') || $this->dashboard !== null;
	}

	protected function doAction() {
		if ($this->dashboard === null) {
			$url = (new CUrl('zabbix.php'))->setArgument('action', 'dashboard.list');
			$this->setResponse((new CControllerResponseRedirect($url->getUrl())));

			return;
		}

		$pageFilter = new CPageFilter([
			'groups' => [
				'monitored_hosts' => true,
				'with_items' => true
			],
			'hosts' => [
				'monitored_hosts' => true,
				'with_items' => true,
				'DDFirstLabel' => _('not selected')
			],
			'hostid' => $this->hasInput('hostid') ? $this->getInput('hostid') : null,
			'groupid' => $this->hasInput('groupid') ? $this->getInput('groupid') : null
		]);

		$data = [
			'dashboard' => $this->dashboard,
			'fullscreen' => $this->getInput('fullscreen', '0'),
			'filter_enabled' => CProfile::get('web.dashconf.filter.enable', 0),
			'grid_widgets' => $this->getWidgets($this->dashboard['widgets']),
			'widget_defaults' => CWidgetConfig::getDefaults(),
			'pageFilter' => $pageFilter,
			'dynamic' => [
				'has_dynamic_widgets' => $this->hasDynamicWidgets($this->dashboard['widgets']),
				'hostid' => $pageFilter->hostid,
				'groupid' => $pageFilter->groupid
			]
		];

		$response = new CControllerResponseData($data);
		$response->setTitle(_('Dashboard'));
		$this->setResponse($response);
	}

	/**
	 * Get dashboard data from API
	 *
	 * @return array|null
	 */
	private function getDashboard() {
		$dashboard = null;

		if ($this->hasInput('new')) {
			$dashboard = $this->getNewDashboard();
		}
		elseif ($this->hasInput('source_dashboardid')) {
			// clone dashboard and show as new
			$dashboards = API::Dashboard()->get([
				'output' => ['name'],
				// TODO AV: remove widgetid from 'selectWidgets'; related CControllerDashbrdWidgetUpdate:155
				'selectWidgets' => ['widgetid', 'type', 'name', 'row', 'col', 'height', 'width', 'fields'],
				'dashboardids' => $this->getInput('source_dashboardid')
			]);

			if ($dashboards) {
				$dashboard = $this->getNewDashboard();
				$dashboard['name'] = $dashboards[0]['name'];
				$dashboard['widgets'] = $dashboards[0]['widgets'];
			}
		}
		else {
			// getting existing dashboard
			$dashboardid = $this->getInput('dashboardid', CProfile::get('web.dashbrd.dashboardid', 0));

			if ($dashboardid == 0 && CProfile::get('web.dashbrd.list_was_opened') != 1) {
				$dashboardid = DASHBOARD_DEFAULT_ID;
			}

			if ($dashboardid != 0) {
				$dashboards = API::Dashboard()->get([
					'output' => ['dashboardid', 'name', 'userid'],
					'selectWidgets' => ['widgetid', 'type', 'name', 'row', 'col', 'height', 'width', 'fields'],
					'dashboardids' => $dashboardid,
					'preservekeys' => true
				]);

				if ($dashboards) {
					$this->prepareEditableFlag($dashboards);
					$dashboard = array_shift($dashboards);
					$dashboard['owner'] = $this->getOwnerData($dashboard['userid']);

					CProfile::update('web.dashbrd.dashboardid', $dashboardid, PROFILE_TYPE_ID);
				}
			}
		}

		return $dashboard;
	}

	/**
	 * Get new dashboard.
	 *
	 * @return array
	 */
	private function getNewDashboard()
	{
		return [
			'dashboardid' => 0,
			'name' => '',
			'editable' => true,
			'widgets' => [],
			'owner' => $this->getOwnerData(CWebUser::$data['userid'])
		];
	}

	/**
	 * Get owner datails.
	 *
	 * @param int $userid
	 *
	 * @return array
	 */
	private function getOwnerData($userid)
	{
		$owner = ['id' => $userid, 'name' => _('Inaccessible user')];

		$users = API::User()->get([
			'output' => ['name', 'surname', 'alias'],
			'userids' => $userid
		]);
		if ($users) {
			$owner['name'] = getUserFullname($users[0]);
		}

		return $owner;
	}

	/**
	 * Get widgets for dashboard
	 *
	 * @return array
	 */
	private function getWidgets($widgets) {
		$grid_widgets = [];

		foreach ($widgets as $widget) {
			$widgetid = $widget['widgetid'];
			$default_rf_rate = CWidgetConfig::getDefaultRfRate($widget['type']);

			$grid_widgets[$widgetid] = [
				'widgetid' => $widgetid,
				'type' => $widget['type'],
				'header' => $widget['name'],
				'pos' => [
					'row' => (int) $widget['row'],
					'col' => (int) $widget['col'],
					'height' => (int) $widget['height'],
					'width' => (int) $widget['width']
				],
				'rf_rate' => (int) CProfile::get('web.dashbrd.widget.rf_rate', $default_rf_rate, $widgetid),
				'fields' => $this->convertWidgetFields($widget['fields'])
			];
		}

		return $grid_widgets;
	}

	/**
	 * Converts fields, received from API to key/value format
	 *
	 * @param array $fields  fields as received from API
	 *
	 * @return array
	 */
	private function convertWidgetFields($fields) {
		$ret = [];
		foreach ($fields as $field) {
			$field_key = CWidgetConfig::getApiFieldKey($field['type']);

			if (array_key_exists($field['name'], $ret)) {
				$ret[$field['name']] = (array) $ret[$field['name']];
				$ret[$field['name']][] = $field[$field_key];
			}
			else {
				$ret[$field['name']] = $field[$field_key];
			}
		}

		return $ret;
	}

	/**
	 * Checks, if any of widgets has checked dynamic field.
	 *
	 * @param array $widgets
	 *
	 * @return bool
	 */
	private function hasDynamicWidgets($widgets) {
		foreach ($widgets as $widget) {
			foreach ($widget['fields'] as $field) {
				// TODO VM: document 'dynamic' as field with special interraction
				if ($field['name'] === 'dynamic' && $field['value_int'] == 1) {
					return true;
				}
			}
		}

		return false;
	}
}
