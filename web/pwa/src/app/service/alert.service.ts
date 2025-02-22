import { AlertController } from '@ionic/angular';
import { EmulationStateService } from './emulation-state.service';
import { Event } from 'microevent.ts';
import { Injectable } from '@angular/core';
import { debounce } from '@pwa/helper/debounce';

@Injectable({
    providedIn: 'root',
})
export class AlertService {
    constructor(private alertController: AlertController, private emulationState: EmulationStateService) {}

    async errorMessage(message: string) {
        const alert = await this.alertController.create({
            header: 'Error',
            backdropDismiss: false,
            message,
            buttons: [{ text: 'Close', role: 'cancel' }],
            cssClass: 'alert-error',
        });

        await alert.present();
    }

    async message(
        header: string,
        message: string,
        extraButtons: Record<string, () => void> = {},
        closeButtonLabel = 'Close'
    ) {
        const alert = await this.alertController.create({
            header,
            message,
            backdropDismiss: false,
            buttons: [
                ...Object.keys(extraButtons).map((text) => ({
                    text,
                    handler: () => {
                        extraButtons[text]();
                        void alert.dismiss();
                    },
                })),
                { text: closeButtonLabel, role: 'cancel' },
            ],
        });

        await alert.present();
        await alert.onDidDismiss();
    }

    @debounce()
    async updateAvailable() {
        const alert = await this.alertController.create({
            header: 'Update available',
            message: 'An update is available. Please close CloudpilotEmu and reload in order to update.',
            backdropDismiss: false,
            buttons: [
                { text: 'Reload', handler: () => window.location.reload() },
                { text: 'Close', role: 'cancel' },
            ],
        });

        await alert.present();
        await alert.onDidDismiss();
    }

    @debounce()
    async fatalError(reason: string, emergencySave = true) {
        const haveCurrentSession = !!this.emulationState.getCurrentSession();

        const alert = await this.alertController.create({
            header: 'Error',
            message: `
                ${reason}
                <br/><br/>
                Please close or reload this window.
                ${
                    haveCurrentSession && emergencySave
                        ? ' You may save an image of your current session before reloading.'
                        : ''
                }
            `,
            backdropDismiss: false,
            buttons: [
                ...(haveCurrentSession && emergencySave
                    ? [
                          {
                              text: 'Save image',
                              handler: () => {
                                  this.emergencySaveEvent.dispatch();
                                  return false;
                              },
                          },
                      ]
                    : []),
                { text: 'Reload', handler: () => window.location.reload() },
            ],
            cssClass: 'alert-error',
        });

        await alert.present();
        await alert.onDidDismiss();
    }

    @debounce()
    async errorInNativeCode(reason: string) {
        const alert = await this.alertController.create({
            header: 'Error',
            message: `
                You encountered a bug in CloudpilotEmu:
                <br/><br/>
                ${reason}.
                <br/><br/>
                Please close or reload this window.
            `,
            backdropDismiss: false,
            buttons: [{ text: 'Reload', handler: () => window.location.reload() }],
        });

        await alert.present();
        await alert.onDidDismiss();
    }

    proxyVersionMismatchError() {
        return this.errorMessage(`
            Server version does not match CloudpilotEmu. Please make sure that you are using the latest
            versions of CloudpilotEmu and of the server.
        `);
    }

    cardHasNoValidFileSystem(): Promise<void> {
        return this.message('No filesystem', 'This card does not contain a valid filesystem.', {}, 'Continue');
    }

    cardHasUncorrectableErrors(): Promise<void> {
        return this.message(
            'Uncorrectable errors',
            'The filesystem on this card contains uncorrectable errors.',
            {},
            'Continue'
        );
    }

    cardIsClean(): Promise<void> {
        return this.message('Card clean', 'No filesystem errors were found.', {}, 'Continue');
    }

    emergencySaveEvent = new Event<void>();
}
