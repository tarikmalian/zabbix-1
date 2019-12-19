<?php
/*
** Zabbix
** Copyright (C) 2001-2019 Zabbix SIA
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


/**
 * Controller class containing operations for adding and updating media type message templates.
 */
class CControllerPopupMediatypeMessage extends CController {

	/**
	 * @var array  An array with all message template types.
	 */
	protected $message_types = [];

	protected function init() {
		$this->disableSIDvalidation();

		$this->message_types = CMediatypeHelper::getAllMessageTypes();
	}

	protected function checkInput() {
		$fields = [
			'type' =>				'in '.implode(',', array_keys(media_type2str())),
			'content_type' =>		'in '.SMTP_MESSAGE_FORMAT_PLAIN_TEXT.','.SMTP_MESSAGE_FORMAT_HTML,
			'message_type' =>		'in '.implode(',', $this->message_types),
			'old_message_type' =>	'in '.implode(',', $this->message_types),
			'message_types' =>		'array',
			'eventsource' =>		'db media_type_message.eventsource|in '.implode(',', [EVENT_SOURCE_TRIGGERS, EVENT_SOURCE_DISCOVERY, EVENT_SOURCE_AUTOREGISTRATION, EVENT_SOURCE_INTERNAL]),
			'recovery' =>			'db media_type_message.recovery|in '.implode(',', [ACTION_OPERATION, ACTION_RECOVERY_OPERATION, ACTION_ACKNOWLEDGE_OPERATION]),
			'subject' =>			'db media_type_message.subject',
			'message' =>			'db media_type_message.message'
		];

		$ret = $this->validateInput($fields);

		if (!$ret) {
			$output = [];

			if (($messages = getMessages()) !== null) {
				$output['errors'] = $messages->toString();
			}

			$this->setResponse(
				(new CControllerResponseData(['main_block' => CJs::encodeJson($output)]))->disableView()
			);
		}

		return $ret;
	}

	protected function checkPermissions() {
		return true;
	}

	protected function doAction() {
		$data = [
			'type' => $this->getInput('type'),
			'content_type' => $this->getInput('content_type'),
			'message_types' => $this->getInput('message_types')
		];

		// Update an existing message template.
		if ($this->hasInput('old_message_type')) {
			// from popup
			if ($this->hasInput('message_type')) {
				$data['message_type'] = $this->getInput('message_type');
			}

			$data['old_message_type'] = $this->getInput('old_message_type');
			$data['subject'] = $this->getInput('subject');
			$data['message'] = $this->getInput('message');


			foreach ($data['message_types'] as $idx => $message_type) {
				if ($message_type == $data['old_message_type']) {
					unset($data['message_types'][$idx]);
				}
			}
		}
		else {
			// Add a new message template.

			// from popup
			if ($this->hasInput('message_type')) {
				$data['message_type'] = $this->getInput('message_type');
				$data['subject'] = $this->getInput('subject');
				$data['message'] = $this->getInput('message');
			}
			else {
				$diff = array_diff($this->message_types, $data['message_types']);
				$diff = reset($diff);
				$data['message_type'] = $diff ? $diff : CMediatypeHelper::MSG_TYPE_PROBLEM;
				$message_template = CMediatypeHelper::getMessageTemplate($data['type'], $data['message_type'],
					$data['content_type']
				);
				$data['subject'] = $message_template['subject'];
				$data['message'] = $message_template['message'];
			}
		}

		// from popup
		if (!$this->getInput('eventsource') && !$this->getInput('recovery')) {
			$from = CMediatypeHelper::transformFromMessageType($data['message_type']);
			$data['eventsource'] = $from['eventsource'];
			$data['recovery'] = $from['recovery'];
		}

		$output = [
			'title' => _('Message template'),
			'params' => $data,
			'user' => [
				'debug_mode' => $this->getDebugMode()
			]
		];

		$this->setResponse(new CControllerResponseData($output));
	}
}
